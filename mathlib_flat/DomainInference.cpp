#include "DomainInference.h"
#include "Numeric.h"

#include <cmath>
#include <limits>

namespace math {

static const double INF = std::numeric_limits<double>::infinity();

namespace {

struct Context {
    std::string var;
    Set domain = Set::reals();
    bool exact = true;
    std::vector<std::string> notes;

    RealFn evalOf(const ExprPtr& e) {
        std::string v = var;
        return [e, v](double x) { return e->eval(Env{{v, x}}); };
    }
};

// require(e >= 0) — used for sqrt, even roots
void requireNonNegative(const ExprPtr& e, Context& ctx) {
    // find zero crossings of e over a wide scan range, then test sign of
    // each resulting sub-interval; keep the ones where e >= 0.
    RealFn f = ctx.evalOf(e);
    std::vector<double> roots = findRootsScan(f, -1000.0, 1000.0, 4000);

    std::vector<double> bounds = {-INF};
    for (double r : roots) bounds.push_back(r);
    bounds.push_back(INF);

    Set allowed = Set::empty();
    for (size_t i = 0; i + 1 < bounds.size(); ++i) {
        double lo = bounds[i], hi = bounds[i + 1];
        double mid = std::isinf(lo) ? (std::isinf(hi) ? 0.0 : hi - 1.0)
                                     : (std::isinf(hi) ? lo + 1.0 : (lo + hi) / 2.0);
        double val;
        try { val = f(mid); } catch (...) { continue; }
        if (std::isnan(val)) continue;
        if (val >= -1e-9) {
            allowed.intervals.push_back({lo, true, hi, true});
        }
    }
    allowed.normalize();
    ctx.domain = ctx.domain.intersect(allowed);
    ctx.notes.push_back("requires " + e->toString() + " >= 0");
}

// require(e > 0) — used for ln
void requirePositive(const ExprPtr& e, Context& ctx) {
    RealFn f = ctx.evalOf(e);
    std::vector<double> roots = findRootsScan(f, -1000.0, 1000.0, 4000);
    std::vector<double> bounds = {-INF};
    for (double r : roots) bounds.push_back(r);
    bounds.push_back(INF);

    Set allowed = Set::empty();
    for (size_t i = 0; i + 1 < bounds.size(); ++i) {
        double lo = bounds[i], hi = bounds[i + 1];
        double mid = std::isinf(lo) ? (std::isinf(hi) ? 0.0 : hi - 1.0)
                                     : (std::isinf(hi) ? lo + 1.0 : (lo + hi) / 2.0);
        double val;
        try { val = f(mid); } catch (...) { continue; }
        if (std::isnan(val)) continue;
        if (val > 1e-9) allowed.intervals.push_back({lo, false, hi, false});
    }
    allowed.normalize();
    ctx.domain = ctx.domain.intersect(allowed);
    ctx.notes.push_back("requires " + e->toString() + " > 0");
}

// require(e != 0) — used for denominators
void requireNonZero(const ExprPtr& e, Context& ctx) {
    RealFn f = ctx.evalOf(e);
    std::vector<double> roots = findRootsScan(f, -1000.0, 1000.0, 4000);
    for (double r : roots) ctx.domain.excludedPoints.push_back(r);
    if (!roots.empty()) ctx.notes.push_back("requires " + e->toString() + " != 0");
    ctx.domain.normalize();
}

// require(-1 <= e <= 1) — used for asin/acos
void requireInUnitRange(const ExprPtr& e, Context& ctx) {
    RealFn f = ctx.evalOf(e);
    std::vector<double> roots1 = findRootsScan([&](double x){ return f(x) - 1.0; }, -1000.0, 1000.0, 4000);
    std::vector<double> rootsM1 = findRootsScan([&](double x){ return f(x) + 1.0; }, -1000.0, 1000.0, 4000);
    std::vector<double> bounds = {-INF};
    for (double r : roots1) bounds.push_back(r);
    for (double r : rootsM1) bounds.push_back(r);
    bounds.push_back(INF);
    std::sort(bounds.begin(), bounds.end());

    Set allowed = Set::empty();
    for (size_t i = 0; i + 1 < bounds.size(); ++i) {
        double lo = bounds[i], hi = bounds[i + 1];
        double mid = std::isinf(lo) ? (std::isinf(hi) ? 0.0 : hi - 1.0)
                                     : (std::isinf(hi) ? lo + 1.0 : (lo + hi) / 2.0);
        double val;
        try { val = f(mid); } catch (...) { continue; }
        if (std::isnan(val)) continue;
        if (val >= -1.0 - 1e-9 && val <= 1.0 + 1e-9) allowed.intervals.push_back({lo, true, hi, true});
    }
    allowed.normalize();
    ctx.domain = ctx.domain.intersect(allowed);
    ctx.notes.push_back("requires -1 <= " + e->toString() + " <= 1");
}

void walk(const ExprPtr& e, Context& ctx) {
    switch (e->kind) {
        case Kind::Number:
        case Kind::Symbol:
            return;
        case Kind::Add:
        case Kind::Mul:
            for (auto& a : e->args) walk(a, ctx);
            return;
        case Kind::Pow: {
            walk(e->args[0], ctx);
            walk(e->args[1], ctx);
            // base^exp: if exponent is a non-integer constant, base must be >= 0
            // (real-valued fractional powers); if exponent is a negative
            // constant, base must be != 0.
            const ExprPtr& base = e->args[0];
            const ExprPtr& exp = e->args[1];
            if (exp->kind == Kind::Number) {
                double n = exp->value;
                bool isInt = std::abs(n - std::round(n)) < 1e-9;
                if (!isInt) {
                    requireNonNegative(base, ctx);
                } else if (n < 0) {
                    requireNonZero(base, ctx);
                }
            } else if (base->contains(ctx.var) == false) {
                // constant^expr — fine for any real exponent as long as base>0,
                // or base==0 is fine too for exp>0; skip strict enforcement here.
            }
            return;
        }
        case Kind::Func: {
            for (auto& a : e->args) walk(a, ctx);
            const ExprPtr& u = e->args[0];
            if (e->name == "sqrt") requireNonNegative(u, ctx);
            else if (e->name == "ln" || (e->name == "log" && e->args.size() == 1)) requirePositive(u, ctx);
            else if (e->name == "log" && e->args.size() == 2) requirePositive(e->args[1], ctx);
            else if (e->name == "asin" || e->name == "acos") requireInUnitRange(u, ctx);
            else if (e->name == "tan") {
                // exclude points where cos(u) == 0
                requireNonZero(Expr::func("cos", u), ctx);
            }
            return;
        }
        case Kind::Call:
            ctx.exact = false;
            ctx.notes.push_back("contains an opaque call to '" + e->name +
                                 "'; its domain restrictions are not accounted for");
            for (auto& a : e->args) walk(a, ctx);
            return;
    }
}

} // namespace

DomainResult inferDomain(const ExprPtr& e, const std::string& var) {
    // also scan for division: a/b becomes Mul(a, Pow(b,-1)) already handled
    // by the Pow case above (negative exponent -> requireNonZero on base).
    Context ctx;
    ctx.var = var;
    walk(e, ctx);
    ctx.domain.normalize();
    return DomainResult{ctx.domain, ctx.exact, ctx.notes};
}

} // namespace math
