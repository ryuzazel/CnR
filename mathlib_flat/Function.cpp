#include "Function.h"
#include "Numeric.h"
#include "Parser.h"

#include <cmath>
#include <sstream>
#include <algorithm>
#include <limits>

namespace math {

static const double FN_INF = std::numeric_limits<double>::infinity();
static bool sameNumberApproxZero(double v);

// ============================================================================
// domain / image
// ============================================================================

Function& Function::setDomain(Set d) { m_domain = std::move(d); m_domainExact = true; return *this; }

static Set parseIntervalSpec(const std::string& spec);

Function& Function::setDomain(const std::string& intervalExpr) {
    m_domain = parseIntervalSpec(intervalExpr);
    m_domainExact = true;
    return *this;
}

Function& Function::setImage(Set i) { m_image = std::move(i); return *this; }
Function& Function::setImage(const std::string& intervalExpr) {
    m_image = parseIntervalSpec(intervalExpr);
    return *this;
}

// Parses simple interval specs: "R", "[a,b]", "(a,b)", "[a,b)", "(a,b]",
// with a,b possibly "inf"/"-inf", and unions joined with "U" or "u" or ",,".
static Set parseIntervalSpec(const std::string& spec) {
    std::string s = spec;
    // normalize
    std::string trimmed;
    for (char c : s) if (!std::isspace((unsigned char)c)) trimmed += c;
    if (trimmed == "R" || trimmed == "RR" || trimmed == "reals") return Set::reals();

    Set result = Set::empty();
    // split on 'U' (union)
    std::vector<std::string> pieces;
    std::string cur;
    for (size_t i = 0; i < trimmed.size(); ++i) {
        if (trimmed[i] == 'U' || trimmed[i] == 'u') { pieces.push_back(cur); cur.clear(); }
        else cur += trimmed[i];
    }
    pieces.push_back(cur);

    for (auto& p : pieces) {
        if (p.empty()) continue;
        bool loInc = p.front() == '[';
        bool hiInc = p.back() == ']';
        std::string inner = p.substr(1, p.size() - 2);
        size_t comma = inner.find(',');
        if (comma == std::string::npos) throw ParseError("bad interval spec: " + p);
        std::string loStr = inner.substr(0, comma);
        std::string hiStr = inner.substr(comma + 1);
        auto parseEndpoint = [](std::string t) -> double {
            if (t == "inf" || t == "+inf") return FN_INF;
            if (t == "-inf") return -FN_INF;
            return std::stod(t);
        };
        double lo = parseEndpoint(loStr);
        double hi = parseEndpoint(hiStr);
        result.intervals.push_back({lo, loInc, hi, hiInc});
    }
    result.normalize();
    return result;
}

void Function::ensureDomain() const {
    if (m_domain) return;
    DomainResult r = inferDomain(body, params.empty() ? "x" : params[0]);
    m_domain = r.domain;
    m_domainExact = r.exact;
}

const Set& Function::domain() const { ensureDomain(); return *m_domain; }
bool Function::domainIsExact() const { ensureDomain(); return m_domainExact; }

RealFnLike Function::evalFn() const {
    std::string p = params.empty() ? "x" : params[0];
    ExprPtr b = body;
    return [b, p](double x) { return b->eval(Env{{p, x}}); };
}

void Function::ensureImage() const {
    if (m_image) return;
    // numeric sampling over the domain to approximate the image: sample
    // densely, track min/max per connected piece, and report the resulting
    // hull as a set of intervals. This is an approximation, not exact CAS
    // range computation.
    ensureDomain();
    RealFnLike f = evalFn();
    Set img = Set::empty();

    for (auto& iv : m_domain->intervals) {
        double lo = iv.lo, hi = iv.hi;
        double scanLo = std::isinf(lo) ? -50.0 : lo;
        double scanHi = std::isinf(hi) ? 50.0 : hi;
        if (scanHi <= scanLo) continue;
        int N = 2000;
        double vmin = FN_INF, vmax = -FN_INF;
        bool any = false;
        for (int i = 0; i <= N; ++i) {
            double x = scanLo + (scanHi - scanLo) * i / N;
            if (m_domain->contains(x) == false) continue;
            double y;
            try { y = f(x); } catch (...) { continue; }
            if (std::isnan(y) || std::isinf(y)) continue;
            vmin = std::min(vmin, y);
            vmax = std::max(vmax, y);
            any = true;
        }
        if (any) img.intervals.push_back({vmin, true, vmax, true});
    }
    img.normalize();
    m_image = img;
}

const Set& Function::image() const { ensureImage(); return *m_image; }

// ============================================================================
// calculus
// ============================================================================

ExprPtr Function::derivative(int order) const {
    std::string p = params.empty() ? "x" : params[0];
    return partialDerivative(p, order);
}

ExprPtr Function::partialDerivative(const std::string& wrt, int order) const {
    ExprPtr d = body;
    for (int i = 0; i < order; ++i) d = d->derivative(wrt)->simplify();
    return d;
}

std::vector<ExprPtr> Function::gradient() const {
    if (params.empty())
        throw SymbolicError("Function::gradient() requires '" + name + "' to have at least one parameter");
    std::vector<ExprPtr> out;
    out.reserve(params.size());
    for (auto& p : params) out.push_back(partialDerivative(p, 1));
    return out;
}

ExprPtr Function::integral() const {
    std::string p = params.empty() ? "x" : params[0];
    return body->integral(p)->simplify();
}

double Function::definiteIntegral(double a, double b) const {
    RealFnLike f = evalFn();
    return numericIntegral(f, a, b);
}

ExprPtr Function::taylorSeries(double at, int terms) const {
    std::string p = params.empty() ? "x" : params[0];
    ExprPtr d = body;
    std::vector<ExprPtr> outTerms;
    double factorial = 1.0;
    for (int n = 0; n < terms; ++n) {
        if (n > 0) factorial *= n;
        double coeff;
        try {
            coeff = d->eval(Env{{p, at}}) / factorial;
        } catch (...) {
            coeff = std::numeric_limits<double>::quiet_NaN();
        }
        if (!std::isnan(coeff) && std::abs(coeff) > 1e-14) {
            ExprPtr xTerm = sameNumberApproxZero(at)
                ? Expr::pow(Expr::sym(p), Expr::num(n))
                : Expr::pow(Expr::sub(Expr::sym(p), Expr::num(at)), Expr::num(n));
            if (n == 0) outTerms.push_back(Expr::num(coeff));
            else outTerms.push_back(Expr::mul(Expr::num(coeff), xTerm));
        }
        if (n + 1 < terms) {
            try { d = d->derivative(p)->simplify(); }
            catch (...) { break; }
        }
    }
    if (outTerms.empty()) return Expr::num(0.0);
    return Expr::add(outTerms)->simplify();
}

static bool sameNumberApproxZero(double v) { return std::abs(v) < 1e-12; }

// ============================================================================
// limits
// ============================================================================

Function::LimitOutcome Function::limit(double at, int direction) const {
    RealFnLike f = evalFn();
    LimitResult r = numericLimit(f, at, direction);
    LimitOutcome out;
    out.exists = r.exists;
    out.value = r.value;
    out.isInfinite = r.isInfinite;
    out.sign = r.signedInfinity;
    std::ostringstream os;
    std::string dirStr = direction < 0 ? "-" : (direction > 0 ? "+" : "");
    if (!r.exists) {
        os << "limit as x -> " << at << (dirStr.empty() ? "" : (dirStr == "-" ? " (from left)" : " (from right)"))
           << " does not appear to exist (or the two sides disagree)";
    } else if (r.isInfinite) {
        os << "limit as x -> " << at << " is " << (r.signedInfinity > 0 ? "+infinity" : "-infinity");
    } else {
        os << "limit as x -> " << at << " = " << r.value;
    }
    out.description = os.str();
    return out;
}

Function::LimitOutcome Function::limitAtInfinity(int direction) const {
    RealFnLike f = evalFn();
    LimitResult r = numericLimitAtInfinity(f, direction);
    LimitOutcome out;
    out.exists = r.exists;
    out.value = r.value;
    out.isInfinite = r.isInfinite;
    out.sign = r.signedInfinity;
    std::ostringstream os;
    os << "limit as x -> " << (direction > 0 ? "+infinity" : "-infinity") << " ";
    if (!r.exists) os << "does not appear to exist / converge";
    else if (r.isInfinite) os << "is " << (r.signedInfinity > 0 ? "+infinity" : "-infinity");
    else os << "= " << r.value;
    out.description = os.str();
    return out;
}

// ============================================================================
// sum / product
// ============================================================================

double Function::sum(long long a, long long b) const {
    std::string p = params.empty() ? "x" : params[0];
    double s = 0.0;
    for (long long k = a; k <= b; ++k) s += body->eval(Env{{p, (double)k}});
    return s;
}

double Function::product(long long a, long long b) const {
    std::string p = params.empty() ? "x" : params[0];
    double s = 1.0;
    for (long long k = a; k <= b; ++k) s *= body->eval(Env{{p, (double)k}});
    return s;
}

// ============================================================================
// evaluation
// ============================================================================

double Function::operator()(double x) const {
    std::string p = params.empty() ? "x" : params[0];
    return body->eval(Env{{p, x}});
}

double Function::eval(const std::vector<double>& argVals) const {
    Env env;
    for (size_t i = 0; i < params.size() && i < argVals.size(); ++i) env[params[i]] = argVals[i];
    return body->eval(env);
}

// ============================================================================
// composition / algebra
// ============================================================================

Function Function::composeWith(const Function& inner) const {
    Function result;
    result.name = name + "_of_" + inner.name;
    result.params = inner.params.empty() ? std::vector<std::string>{"x"} : inner.params;
    std::string myParam = params.empty() ? "x" : params[0];
    result.body = body->substitute(myParam, inner.body)->simplify();
    return result;
}

Function Function::plus(const Function& other) const {
    Function result;
    result.name = "(" + name + "+" + other.name + ")";
    result.params = params;
    result.body = Expr::add(body, other.body)->simplify();
    return result;
}

Function Function::minus(const Function& other) const {
    Function result;
    result.name = "(" + name + "-" + other.name + ")";
    result.params = params;
    result.body = Expr::sub(body, other.body)->simplify();
    return result;
}

Function Function::times(const Function& other) const {
    Function result;
    result.name = "(" + name + "*" + other.name + ")";
    result.params = params;
    result.body = Expr::mul(body, other.body)->simplify();
    return result;
}

// ============================================================================
// rules
// ============================================================================

Function& Function::addRule(const std::string& description, const std::string& patternExpr, const std::string& replacementExpr) {
    Rule r;
    r.description = description;
    r.pattern = Parser::parseExpression(patternExpr);
    r.replacement = Parser::parseExpression(replacementExpr);
    m_rules.push_back(r);
    return *this;
}

Function& Function::addRule(Rule r) { m_rules.push_back(std::move(r)); return *this; }

ExprPtr Function::applyRules(const ExprPtr& e, bool repeat, int maxPasses) const {
    ExprPtr cur = e;
    for (int pass = 0; pass < maxPasses; ++pass) {
        bool changed = false;
        for (auto& r : m_rules) {
            ExprPtr next = cur->substitute(r.pattern, r.replacement);
            if (!next->equalsStruct(cur)) { cur = next->simplify(); changed = true; }
        }
        if (!repeat || !changed) break;
    }
    return cur;
}

// ============================================================================
// properties
// ============================================================================

Function& Function::assertProperty(Property p, const std::string& note) {
    PropertyFact f; f.prop = p; f.note = note;
    m_properties.push_back(f);
    return *this;
}

Function& Function::assertPeriodic(double period, const std::string& note) {
    PropertyFact f; f.prop = Property::Periodic; f.period = period; f.note = note;
    m_properties.push_back(f);
    return *this;
}

bool Function::hasProperty(Property p) const {
    for (auto& f : m_properties) if (f.prop == p) return true;
    return false;
}

std::optional<double> Function::period() const {
    for (auto& f : m_properties) if (f.prop == Property::Periodic && f.period) return f.period;
    return std::nullopt;
}

bool Function::isMonotonic(bool& increasing, int samples) const {
    ensureDomain();
    RealFnLike f = evalFn();
    // sample across the (bounded portion of the) domain, tracking sign of
    // consecutive differences
    std::vector<double> xs;
    for (auto& iv : m_domain->intervals) {
        double lo = std::isinf(iv.lo) ? -20.0 : iv.lo;
        double hi = std::isinf(iv.hi) ? 20.0 : iv.hi;
        if (hi <= lo) continue;
        for (int i = 0; i <= samples; ++i) {
            double x = lo + (hi - lo) * i / samples;
            if (m_domain->contains(x)) xs.push_back(x);
        }
    }
    std::sort(xs.begin(), xs.end());
    xs.erase(std::unique(xs.begin(), xs.end()), xs.end());
    if (xs.size() < 3) return false;

    std::vector<double> ys;
    for (double x : xs) {
        double v;
        try { v = f(x); } catch (...) { v = std::numeric_limits<double>::quiet_NaN(); }
        if (std::isinf(v)) v = std::numeric_limits<double>::quiet_NaN();
        ys.push_back(v);
    }

    int incCount = 0, decCount = 0, total = 0;
    for (size_t i = 1; i < ys.size(); ++i) {
        if (std::isnan(ys[i]) || std::isnan(ys[i-1])) continue;
        total++;
        if (ys[i] > ys[i-1]) incCount++;
        else if (ys[i] < ys[i-1]) decCount++;
    }
    if (total == 0) return false;
    if (incCount == total) { increasing = true; return true; }
    if (decCount == total) { increasing = false; return true; }
    return false;
}

bool Function::isBijective(int samples) const {
    if (hasProperty(Property::Bijective)) return true;
    bool increasing;
    // strictly monotonic on its domain implies injective; combined with the
    // (numerically estimated) image as codomain, that's bijective onto it.
    return isMonotonic(increasing, samples);
}

Function Function::inverse() const {
    if (!isBijective()) {
        throw SymbolicError("Function::inverse() requires the function to be bijective "
                             "(monotonic) on its domain; '" + name + "' does not appear to be. "
                             "If you know it is (e.g. on a restricted domain), call "
                             "assertProperty(Property::Bijective) first.");
    }
    std::string p = params.empty() ? "x" : params[0];

    // Try a few common closed forms first: a*x+b ; a*x^n+b (n odd, or domain
    // restricted to x>=0) ; a*e^x+b ; a*ln(x)+b.
    ExprPtr b = body->simplify();

    // a*x + b  (linear)
    {
        // detect via probing: f(0), f(1) -> a = f(1)-f(0), b = f(0)
        try {
            double f0 = (*this)(0.0);
            double f1 = (*this)(1.0);
            double f2 = (*this)(2.0);
            double a = f1 - f0;
            if (std::abs((f2 - f1) - a) < 1e-9 && std::abs(a) > 1e-12) {
                // f(x) = a*x + f0  =>  f^-1(y) = (y - f0)/a
                Function inv;
                inv.name = name + "_inverse";
                inv.params = {"x"};
                inv.body = Expr::div(Expr::sub(Expr::sym("x"), Expr::num(f0)), Expr::num(a))->simplify();
                return inv;
            }
        } catch (...) {}
    }

    // a*e^x + b  or a*ln(x)+b: detected by shape of the AST directly.
    if (b->kind == Kind::Add && b->args.size() == 2) {
        for (int i = 0; i < 2; ++i) {
            ExprPtr term = b->args[i];
            ExprPtr rest = b->args[1 - i];
            if (!rest->contains(p) && term->contains(p)) {
                // term should be a*e^x or a*ln(x)
                ExprPtr coeff = Expr::num(1.0);
                ExprPtr core = term;
                if (term->kind == Kind::Mul) {
                    std::vector<ExprPtr> consts, nonconsts;
                    for (auto& f : term->args) (f->contains(p) ? nonconsts : consts).push_back(f);
                    if (nonconsts.size() == 1) {
                        coeff = consts.empty() ? Expr::num(1.0) : Expr::mul(consts);
                        core = nonconsts[0];
                    }
                }
                if (core->kind == Kind::Func && core->name == "exp" && core->args[0]->isSymbol(p)) {
                    // y = a*e^x + b  =>  x = ln((y-b)/a)
                    Function inv;
                    inv.name = name + "_inverse";
                    inv.params = {"x"};
                    inv.body = Expr::func("ln", Expr::div(Expr::sub(Expr::sym("x"), rest), coeff))->simplify();
                    return inv;
                }
                if (core->kind == Kind::Func && core->name == "ln" && core->args[0]->isSymbol(p)) {
                    // y = a*ln(x) + b  =>  x = e^((y-b)/a)
                    Function inv;
                    inv.name = name + "_inverse";
                    inv.params = {"x"};
                    inv.body = Expr::func("exp", Expr::div(Expr::sub(Expr::sym("x"), rest), coeff))->simplify();
                    return inv;
                }
            }
        }
    }

    // No closed form matched. The caller can still get inverse values one
    // at a time via numericInverseAt(y), which bisects on the domain.
    throw SymbolicError("no closed-form inverse rule matched for '" + name +
                         "'; the function was confirmed bijective, so you can still call "
                         "numericInverseAt(y) to evaluate the inverse numerically at a point");
}

double Function::numericInverseAt(double y) const {
    if (!isBijective()) {
        throw SymbolicError("numericInverseAt() requires '" + name + "' to be bijective on its domain");
    }
    ensureDomain();
    RealFnLike f = evalFn();
    RealFnLike g = [&](double x) { return f(x) - y; };

    for (auto& iv : m_domain->intervals) {
        double lo = std::isinf(iv.lo) ? -1e6 : iv.lo;
        double hi = std::isinf(iv.hi) ? 1e6 : iv.hi;
        if (hi <= lo) continue;
        auto roots = findRootsScan(g, lo, hi, 4000);
        if (!roots.empty()) return roots.front();
    }
    throw EvalError("no x found with f(x) = " + std::to_string(y) + " for '" + name + "' on its domain");
}

std::string Function::toString() const {
    std::ostringstream os;
    os << name << "(";
    for (size_t i = 0; i < params.size(); ++i) { if (i) os << ", "; os << params[i]; }
    os << ") = " << body->toString();
    return os.str();
}

} // namespace math
