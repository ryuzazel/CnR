// Calculus.cpp — symbolic derivative() and integral() for Expr.
#include "Expr.h"

#include <cmath>

namespace math {

static bool sameNumberHelper(double a, double b) { return std::abs(a - b) < 1e-9; }

// ============================================================================
// derivative
// ============================================================================

static ExprPtr d(const ExprPtr& e, const std::string& v) { return e->derivative(v); }

ExprPtr Expr::derivative(const std::string& wrt) const {
    switch (kind) {
        case Kind::Number:
            return Expr::num(0.0);

        case Kind::Symbol:
            return Expr::num(name == wrt ? 1.0 : 0.0);

        case Kind::Add: {
            std::vector<ExprPtr> terms;
            for (auto& a : args) terms.push_back(d(a, wrt));
            return Expr::add(terms)->simplify();
        }

        case Kind::Mul: {
            // generalized product rule over n factors:
            // d(f1*f2*...*fn) = sum_i [ d(fi) * prod_{j!=i} fj ]
            std::vector<ExprPtr> sumTerms;
            for (size_t i = 0; i < args.size(); ++i) {
                std::vector<ExprPtr> factors;
                factors.push_back(d(args[i], wrt));
                for (size_t j = 0; j < args.size(); ++j)
                    if (j != i) factors.push_back(args[j]);
                sumTerms.push_back(Expr::mul(factors));
            }
            return Expr::add(sumTerms)->simplify();
        }

        case Kind::Pow: {
            const ExprPtr& base = args[0];
            const ExprPtr& exp = args[1];
            bool baseHasV = base->contains(wrt);
            bool expHasV = exp->contains(wrt);

            if (!baseHasV && !expHasV) return Expr::num(0.0);

            if (!expHasV) {
                // power rule: d(base^n) = n * base^(n-1) * d(base)
                ExprPtr newExp = Expr::sub(exp, Expr::num(1.0));
                return Expr::mul({exp, Expr::pow(base, newExp), d(base, wrt)})->simplify();
            }
            if (!baseHasV) {
                // exponential rule: d(a^u) = a^u * ln(a) * d(u)
                return Expr::mul({self(), Expr::func("ln", base), d(exp, wrt)})->simplify();
            }
            // general case: d(base^exp) = base^exp * d(exp*ln(base))
            ExprPtr inner = Expr::mul(exp, Expr::func("ln", base));
            return Expr::mul({self(), d(inner, wrt)})->simplify();
        }

        case Kind::Func: {
            const ExprPtr& u = args[0];
            ExprPtr du = d(u, wrt);
            ExprPtr chain;

            if (name == "sin") chain = Expr::func("cos", u);
            else if (name == "cos") chain = Expr::neg(Expr::func("sin", u));
            else if (name == "tan") chain = Expr::pow(Expr::func("cos", u), Expr::num(-2.0));
            else if (name == "exp") chain = self();
            else if (name == "ln") chain = Expr::pow(u, Expr::num(-1.0));
            else if (name == "sqrt") chain = Expr::div(Expr::num(1.0), Expr::mul(Expr::num(2.0), self()));
            else if (name == "asin") chain = Expr::pow(Expr::sub(Expr::num(1.0), Expr::pow(u, Expr::num(2.0))), Expr::num(-0.5));
            else if (name == "acos") chain = Expr::neg(Expr::pow(Expr::sub(Expr::num(1.0), Expr::pow(u, Expr::num(2.0))), Expr::num(-0.5)));
            else if (name == "atan") chain = Expr::pow(Expr::add(Expr::num(1.0), Expr::pow(u, Expr::num(2.0))), Expr::num(-1.0));
            else if (name == "sinh") chain = Expr::func("cosh", u);
            else if (name == "cosh") chain = Expr::func("sinh", u);
            else if (name == "tanh") chain = Expr::sub(Expr::num(1.0), Expr::pow(Expr::func("tanh", u), Expr::num(2.0)));
            else if (name == "abs") chain = Expr::func("sign", u);
            else
                throw SymbolicError("no derivative rule known for function '" + name + "'");

            return Expr::mul(chain, du)->simplify();
        }

        case Kind::Call:
            throw SymbolicError("cannot differentiate opaque call to '" + name +
                                 "' — expand it via Function::body first");
    }
    throw SymbolicError("unhandled expr kind in derivative()");
}

// ============================================================================
// integral (best-effort symbolic antiderivative)
// ============================================================================
//
// Strategy: this is NOT a full Risch algorithm. It covers, recursively:
//   - sums (linearity)
//   - constant multiples
//   - polynomials (power rule, including x^-1 -> ln|x|)
//   - a table of elementary forms sin/cos/tan/exp/ln/1/(1+x^2)/1/sqrt(1-x^2)
//   - linear substitution: f(a*x+b) when f has a known antiderivative
// Anything outside that raises SymbolicError so callers can fall back
// to numeric integration.

static bool isLinear(const ExprPtr& e, const std::string& v, double& a, double& b) {
    // recognizes a*v + b (a,b constants), a bit loosely via simplify+eval trick
    try {
        ExprPtr s = e->simplify();
        double f0 = s->eval(Env{{v, 0.0}});
        double f1 = s->eval(Env{{v, 1.0}});
        double f2 = s->eval(Env{{v, 2.0}});
        double slope1 = f1 - f0;
        double slope2 = f2 - f1;
        if (std::abs(slope1 - slope2) > 1e-9) return false; // not linear
        a = slope1;
        b = f0;
        return true;
    } catch (...) {
        return false;
    }
}

ExprPtr Expr::integral(const std::string& wrt) const {
    switch (kind) {
        case Kind::Number:
            return Expr::mul(self(), Expr::sym(wrt))->simplify();

        case Kind::Symbol:
            if (name == wrt) return Expr::div(Expr::pow(self(), Expr::num(2.0)), Expr::num(2.0))->simplify();
            // constant symbol w.r.t. wrt
            return Expr::mul(self(), Expr::sym(wrt))->simplify();

        case Kind::Add: {
            std::vector<ExprPtr> terms;
            for (auto& a : args) terms.push_back(a->integral(wrt));
            return Expr::add(terms)->simplify();
        }

        case Kind::Mul: {
            // pull out constant factors (those not containing wrt)
            std::vector<ExprPtr> constFactors, varFactors;
            for (auto& f : args) {
                if (f->contains(wrt)) varFactors.push_back(f);
                else constFactors.push_back(f);
            }
            if (varFactors.empty()) {
                return Expr::mul({Expr::mul(constFactors), Expr::sym(wrt)})->simplify();
            }
            if (varFactors.size() == 1) {
                ExprPtr inner = varFactors[0]->integral(wrt);
                constFactors.push_back(inner);
                return Expr::mul(constFactors)->simplify();
            }
            throw SymbolicError("integral of a product of multiple non-constant factors "
                                 "is not supported symbolically (needs substitution/parts)");
        }

        case Kind::Pow: {
            const ExprPtr& base = args[0];
            const ExprPtr& exp = args[1];
            if (base->isSymbol(wrt) && !exp->contains(wrt)) {
                if (exp->kind == Kind::Number && sameNumberHelper(exp->value, -1.0)) {
                    return Expr::func("ln", Expr::func("abs", base));
                }
                ExprPtr newExp = Expr::add(exp, Expr::num(1.0));
                return Expr::div(Expr::pow(base, newExp), newExp)->simplify();
            }
            // a^x with constant a, variable exponent x = wrt
            if (!base->contains(wrt) && exp->isSymbol(wrt)) {
                return Expr::div(self(), Expr::func("ln", base))->simplify();
            }
            // linear substitution: (a*x+b)^n
            double a, b;
            if (isLinear(base, wrt, a, b) && !exp->contains(wrt) && std::abs(a) > 1e-12) {
                if (exp->kind == Kind::Number && sameNumberHelper(exp->value, -1.0)) {
                    return Expr::div(Expr::func("ln", Expr::func("abs", base)), Expr::num(a));
                }
                ExprPtr newExp = Expr::add(exp, Expr::num(1.0));
                return Expr::div(Expr::pow(base, newExp), Expr::mul(Expr::num(a), newExp))->simplify();
            }
            throw SymbolicError("no antiderivative rule for this power expression");
        }

        case Kind::Func: {
            const ExprPtr& u = args[0];
            double a, b;
            bool linear = isLinear(u, wrt, a, b);

            auto wrap = [&](ExprPtr F) -> ExprPtr {
                // if u = a*x+b, antiderivative of f(u) is F(u)/a
                if (u->isSymbol(wrt)) return F;
                if (linear && std::abs(a) > 1e-12) return Expr::div(F, Expr::num(a))->simplify();
                throw SymbolicError("integral of '" + name + "' with non-linear inner "
                                     "expression is not supported symbolically");
            };

            if (name == "sin") return wrap(Expr::neg(Expr::func("cos", u)));
            if (name == "cos") return wrap(Expr::func("sin", u));
            if (name == "exp") return wrap(Expr::func("exp", u));
            if (name == "ln")  {
                // integral of ln(x) dx = x*ln(x) - x  (only for simple x, i.e. linear u handled via wrap-like combo)
                if (u->isSymbol(wrt)) {
                    return Expr::sub(Expr::mul(u, Expr::func("ln", u)), u)->simplify();
                }
                throw SymbolicError("integral of ln(u) for non-trivial u is not supported symbolically");
            }
            if (name == "tan") return wrap(Expr::neg(Expr::func("ln", Expr::func("abs", Expr::func("cos", u)))));

            throw SymbolicError("no antiderivative rule known for function '" + name + "'");
        }

        case Kind::Call:
            throw SymbolicError("cannot integrate opaque call to '" + name +
                                 "' — expand it via Function::body first");
    }
    throw SymbolicError("unhandled expr kind in integral()");
}

} // namespace math
