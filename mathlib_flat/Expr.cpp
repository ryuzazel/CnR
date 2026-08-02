#include "Expr.h"

#include <cmath>
#include <sstream>
#include <algorithm>
#include <set>

namespace math {

// ============================================================================
// Construction helpers
// ============================================================================

ExprPtr Expr::num(double v) {
    auto e = std::make_shared<Expr>(Kind::Number);
    e->value = v;
    return e;
}

ExprPtr Expr::sym(const std::string& n) {
    auto e = std::make_shared<Expr>(Kind::Symbol);
    e->name = n;
    return e;
}

ExprPtr Expr::add(std::vector<ExprPtr> xs) {
    auto e = std::make_shared<Expr>(Kind::Add);
    e->args = std::move(xs);
    return e;
}

ExprPtr Expr::add(ExprPtr a, ExprPtr b) { return add(std::vector<ExprPtr>{a, b}); }

ExprPtr Expr::sub(ExprPtr a, ExprPtr b) { return add(a, neg(b)); }

ExprPtr Expr::neg(ExprPtr a) { return mul(num(-1.0), a); }

ExprPtr Expr::mul(std::vector<ExprPtr> xs) {
    auto e = std::make_shared<Expr>(Kind::Mul);
    e->args = std::move(xs);
    return e;
}

ExprPtr Expr::mul(ExprPtr a, ExprPtr b) { return mul(std::vector<ExprPtr>{a, b}); }

ExprPtr Expr::div(ExprPtr a, ExprPtr b) { return mul(a, pow(b, num(-1.0))); }

ExprPtr Expr::pow(ExprPtr base, ExprPtr exp) {
    auto e = std::make_shared<Expr>(Kind::Pow);
    e->args = {base, exp};
    return e;
}

ExprPtr Expr::func(const std::string& fname, ExprPtr arg) {
    auto e = std::make_shared<Expr>(Kind::Func);
    e->name = fname;
    e->args = {arg};
    return e;
}

ExprPtr Expr::func(const std::string& fname, ExprPtr a, ExprPtr b) {
    auto e = std::make_shared<Expr>(Kind::Func);
    e->name = fname;
    e->args = {a, b};
    return e;
}

ExprPtr Expr::call(const std::string& fname, std::vector<ExprPtr> callArgs) {
    auto e = std::make_shared<Expr>(Kind::Call);
    e->name = fname;
    e->args = std::move(callArgs);
    return e;
}

ExprPtr operator+(ExprPtr a, ExprPtr b) { return Expr::add(a, b); }
ExprPtr operator-(ExprPtr a, ExprPtr b) { return Expr::sub(a, b); }
ExprPtr operator*(ExprPtr a, ExprPtr b) { return Expr::mul(a, b); }
ExprPtr operator/(ExprPtr a, ExprPtr b) { return Expr::div(a, b); }
ExprPtr operator-(ExprPtr a) { return Expr::neg(a); }

// ============================================================================
// Queries
// ============================================================================

bool Expr::isNumber(double v) const {
    return kind == Kind::Number && std::abs(value - v) < 1e-12;
}

bool Expr::contains(const std::string& v) const {
    if (kind == Kind::Symbol) return name == v;
    for (auto& a : args) if (a->contains(v)) return true;
    return false;
}

static bool sameNumber(double a, double b) { return std::abs(a - b) < 1e-9; }

bool Expr::equalsStruct(const ExprPtr& other) const {
    if (kind != other->kind) return false;
    switch (kind) {
        case Kind::Number: return sameNumber(value, other->value);
        case Kind::Symbol: return name == other->name;
        case Kind::Func:
        case Kind::Call:
            if (name != other->name) return false;
            if (args.size() != other->args.size()) return false;
            for (size_t i = 0; i < args.size(); ++i)
                if (!args[i]->equalsStruct(other->args[i])) return false;
            return true;
        case Kind::Pow:
            return args[0]->equalsStruct(other->args[0]) &&
                   args[1]->equalsStruct(other->args[1]);
        case Kind::Add:
        case Kind::Mul: {
            // order-independent comparison (n-ary, commutative)
            if (args.size() != other->args.size()) return false;
            std::vector<bool> used(other->args.size(), false);
            for (auto& a : args) {
                bool found = false;
                for (size_t j = 0; j < other->args.size(); ++j) {
                    if (used[j]) continue;
                    if (a->equalsStruct(other->args[j])) { used[j] = true; found = true; break; }
                }
                if (!found) return false;
            }
            return true;
        }
    }
    return false;
}

// ============================================================================
// toString
// ============================================================================

static std::string numToStr(double v) {
    if (sameNumber(v, std::round(v))) {
        std::ostringstream os;
        os << (long long)std::llround(v);
        return os.str();
    }
    std::ostringstream os;
    os << v;
    return os.str();
}

// crude precedence for parenthesization when printing
static int precedence(const ExprPtr& e) {
    switch (e->kind) {
        case Kind::Add: return 1;
        case Kind::Mul: return 2;
        case Kind::Pow: return 4;
        default: return 5;
    }
}

static std::string toStringPrec(const ExprPtr& e, int parentPrec);

static std::string joinAdd(const ExprPtr& e) {
    std::string out;
    for (size_t i = 0; i < e->args.size(); ++i) {
        auto& t = e->args[i];
        // detect "negative term" = Mul(-1, ...) or Number<0, to print as " - x"
        bool neg = false;
        ExprPtr printTerm = t;
        if (t->kind == Kind::Number && t->value < 0) {
            neg = true;
            printTerm = Expr::num(-t->value);
        } else if (t->kind == Kind::Mul && !t->args.empty() &&
                   t->args[0]->kind == Kind::Number && t->args[0]->value < 0) {
            neg = true;
            std::vector<ExprPtr> rest(t->args.begin() + 1, t->args.end());
            double coeff = -t->args[0]->value;
            if (sameNumber(coeff, 1.0)) {
                printTerm = rest.size() == 1 ? rest[0] : Expr::mul(rest);
            } else {
                rest.insert(rest.begin(), Expr::num(coeff));
                printTerm = Expr::mul(rest);
            }
        }
        std::string s = toStringPrec(printTerm, 1);
        if (i == 0) {
            out += (neg ? "-" : "") + s;
        } else {
            out += (neg ? " - " : " + ") + s;
        }
    }
    return out;
}

static std::string joinMul(const ExprPtr& e) {
    // Print division nicely: separate positive-power and negative-power factors
    std::vector<ExprPtr> num, den;
    for (auto& f : e->args) {
        if (f->kind == Kind::Pow && f->args[1]->kind == Kind::Number && f->args[1]->value < 0) {
            double newExp = -f->args[1]->value;
            den.push_back(sameNumber(newExp, 1.0) ? f->args[0] : Expr::pow(f->args[0], Expr::num(newExp)));
        } else {
            num.push_back(f);
        }
    }
    std::string numStr;
    if (num.empty()) numStr = "1";
    else {
        for (size_t i = 0; i < num.size(); ++i) {
            if (i) numStr += "*";
            numStr += toStringPrec(num[i], 2);
        }
    }
    if (den.empty()) return numStr;
    std::string denStr;
    for (size_t i = 0; i < den.size(); ++i) {
        if (i) denStr += "*";
        denStr += toStringPrec(den[i], 2);
    }
    if (den.size() > 1) denStr = "(" + denStr + ")";
    return numStr + "/" + denStr;
}

static std::string toStringPrec(const ExprPtr& e, int parentPrec) {
    std::string s;
    switch (e->kind) {
        case Kind::Number:
            s = numToStr(e->value);
            break;
        case Kind::Symbol:
            s = e->name;
            break;
        case Kind::Add:
            s = joinAdd(e);
            break;
        case Kind::Mul:
            s = joinMul(e);
            break;
        case Kind::Pow: {
            std::string b = toStringPrec(e->args[0], 4);
            std::string p = toStringPrec(e->args[1], 4);
            s = b + "^" + p;
            break;
        }
        case Kind::Func: {
            s = e->name + "(";
            for (size_t i = 0; i < e->args.size(); ++i) {
                if (i) s += ", ";
                s += toStringPrec(e->args[i], 0);
            }
            s += ")";
            break;
        }
        case Kind::Call: {
            s = e->name + "(";
            for (size_t i = 0; i < e->args.size(); ++i) {
                if (i) s += ", ";
                s += toStringPrec(e->args[i], 0);
            }
            s += ")";
            break;
        }
    }
    if (precedence(e) < parentPrec) s = "(" + s + ")";
    return s;
}

std::string Expr::toString() const { return toStringPrec(self(), 0); }

// ============================================================================
// simplify
// ============================================================================

static bool isConst(const ExprPtr& e) { return e->kind == Kind::Number; }

static ExprPtr simplifyAdd(std::vector<ExprPtr> terms);
static ExprPtr simplifyMul(std::vector<ExprPtr> factors);

// Flatten nested Add/Mul of the same kind, collect numeric constants.
static ExprPtr simplifyAdd(std::vector<ExprPtr> terms) {
    std::vector<ExprPtr> flat;
    for (auto& t : terms) {
        if (t->kind == Kind::Add) for (auto& s : t->args) flat.push_back(s);
        else flat.push_back(t);
    }

    double constSum = 0.0;
    std::vector<ExprPtr> rest;
    for (auto& t : flat) {
        if (isConst(t)) constSum += t->value;
        else rest.push_back(t);
    }

    // combine like terms: represent each non-const term as (coeff, base)
    // base = term with any leading numeric factor stripped.
    struct Term { double coeff; ExprPtr base; };
    std::vector<Term> combined;

    auto splitCoeff = [](ExprPtr t) -> std::pair<double, ExprPtr> {
        if (t->kind == Kind::Mul && !t->args.empty() && t->args[0]->kind == Kind::Number) {
            std::vector<ExprPtr> rest2(t->args.begin() + 1, t->args.end());
            ExprPtr base = rest2.size() == 1 ? rest2[0] : Expr::mul(rest2);
            return {t->args[0]->value, base};
        }
        return {1.0, t};
    };

    for (auto& t : rest) {
        auto [coeff, base] = splitCoeff(t);
        bool merged = false;
        for (auto& c : combined) {
            if (c.base->equalsStruct(base)) { c.coeff += coeff; merged = true; break; }
        }
        if (!merged) combined.push_back({coeff, base});
    }

    std::vector<ExprPtr> outTerms;
    if (!sameNumber(constSum, 0.0) || combined.empty()) outTerms.push_back(Expr::num(constSum));
    for (auto& c : combined) {
        if (sameNumber(c.coeff, 0.0)) continue;
        if (sameNumber(c.coeff, 1.0)) outTerms.push_back(c.base);
        else outTerms.push_back(Expr::mul(Expr::num(c.coeff), c.base));
    }

    if (outTerms.empty()) return Expr::num(0.0);
    if (outTerms.size() == 1) return outTerms[0];
    return Expr::add(outTerms);
}

static ExprPtr simplifyMul(std::vector<ExprPtr> factors) {
    std::vector<ExprPtr> flat;
    for (auto& f : factors) {
        if (f->kind == Kind::Mul) for (auto& s : f->args) flat.push_back(s);
        else flat.push_back(f);
    }

    // Distribute when exactly one factor is a sum: k * (a + b + ...) -> k*a + k*b + ...
    // (Distributing when there are two or more Add factors would blow up into a
    // full polynomial expansion; we keep that as an explicit opt-in via expand(),
    // and only auto-distribute the common "scalar/monomial times a sum" case.)
    {
        int addIdx = -1;
        int addCount = 0;
        for (size_t i = 0; i < flat.size(); ++i) {
            if (flat[i]->kind == Kind::Add) { addIdx = (int)i; addCount++; }
        }
        if (addCount == 1) {
            std::vector<ExprPtr> others;
            for (size_t i = 0; i < flat.size(); ++i) if ((int)i != addIdx) others.push_back(flat[i]);
            std::vector<ExprPtr> distributed;
            for (auto& term : flat[addIdx]->args) {
                std::vector<ExprPtr> f2 = others;
                f2.push_back(term);
                distributed.push_back(simplifyMul(f2));
            }
            return simplifyAdd(distributed);
        }
    }

    double constProd = 1.0;
    std::vector<ExprPtr> rest;
    for (auto& f : flat) {
        if (isConst(f)) constProd *= f->value;
        else rest.push_back(f);
    }
    if (sameNumber(constProd, 0.0)) return Expr::num(0.0);

    // combine powers of identical bases: base^e1 * base^e2 -> base^(e1+e2)
    struct PowTerm { ExprPtr base; std::vector<ExprPtr> exps; };
    std::vector<PowTerm> powers;

    auto baseExp = [](ExprPtr f) -> std::pair<ExprPtr, ExprPtr> {
        if (f->kind == Kind::Pow) return {f->args[0], f->args[1]};
        return {f, Expr::num(1.0)};
    };

    for (auto& f : rest) {
        auto [b, e] = baseExp(f);
        bool merged = false;
        for (auto& p : powers) {
            if (p.base->equalsStruct(b)) { p.exps.push_back(e); merged = true; break; }
        }
        if (!merged) powers.push_back({b, {e}});
    }

    std::vector<ExprPtr> outFactors;
    if (!sameNumber(constProd, 1.0) || powers.empty()) outFactors.push_back(Expr::num(constProd));

    for (auto& p : powers) {
        ExprPtr exp = p.exps.size() == 1 ? p.exps[0] : simplifyAdd(p.exps)->simplify();
        if (exp->isZero()) continue; // base^0 = 1
        if (exp->isOne()) outFactors.push_back(p.base);
        else outFactors.push_back(Expr::pow(p.base, exp));
    }

    if (outFactors.empty()) return Expr::num(1.0);
    if (outFactors.size() == 1) return outFactors[0];
    return Expr::mul(outFactors);
}

ExprPtr Expr::simplify() const {
    switch (kind) {
        case Kind::Number:
        case Kind::Symbol:
            return self();

        case Kind::Add: {
            std::vector<ExprPtr> simplified;
            for (auto& a : args) simplified.push_back(a->simplify());
            return simplifyAdd(simplified);
        }

        case Kind::Mul: {
            std::vector<ExprPtr> simplified;
            for (auto& a : args) simplified.push_back(a->simplify());
            return simplifyMul(simplified);
        }

        case Kind::Pow: {
            ExprPtr base = args[0]->simplify();
            ExprPtr exp = args[1]->simplify();
            if (exp->isZero()) return Expr::num(1.0);
            if (exp->isOne()) return base;
            if (base->isZero()) return Expr::num(0.0);
            if (base->isOne()) return Expr::num(1.0);
            if (isConst(base) && isConst(exp)) return Expr::num(std::pow(base->value, exp->value));
            // (a^b)^c -> a^(b*c)
            if (base->kind == Kind::Pow) {
                return Expr::pow(base->args[0], simplifyMul({base->args[1], exp}))->simplify();
            }
            return Expr::pow(base, exp);
        }

        case Kind::Func: {
            std::vector<ExprPtr> simplified;
            for (auto& a : args) simplified.push_back(a->simplify());
            if (simplified.size() == 1 && isConst(simplified[0])) {
                double x = simplified[0]->value;
                const std::string& f = name;
                if (f == "sin") return Expr::num(std::sin(x));
                if (f == "cos") return Expr::num(std::cos(x));
                if (f == "tan") return Expr::num(std::tan(x));
                if (f == "exp") return Expr::num(std::exp(x));
                if (f == "ln" && x > 0) return Expr::num(std::log(x));
                if (f == "sqrt" && x >= 0) return Expr::num(std::sqrt(x));
                if (f == "abs") return Expr::num(std::abs(x));
            }
            auto e = std::make_shared<Expr>(Kind::Func);
            e->name = name;
            e->args = simplified;
            return e;
        }

        case Kind::Call: {
            std::vector<ExprPtr> simplified;
            for (auto& a : args) simplified.push_back(a->simplify());
            auto e = std::make_shared<Expr>(Kind::Call);
            e->name = name;
            e->args = simplified;
            return e;
        }
    }
    return self();
}

// ============================================================================
// expand — full distribution, (a+b)*(c+d) -> a*c+a*d+b*c+b*d
// ============================================================================

static ExprPtr expandMulOfSums(const std::vector<ExprPtr>& factors) {
    // cartesian-product distribution across all Add factors
    std::vector<std::vector<ExprPtr>> sumTermLists; // one list per Add factor
    std::vector<ExprPtr> nonAdd;
    for (auto& f : factors) {
        if (f->kind == Kind::Add) sumTermLists.push_back(f->args);
        else nonAdd.push_back(f);
    }
    if (sumTermLists.empty()) return Expr::mul(nonAdd)->simplify();

    std::vector<std::vector<ExprPtr>> combos = {{}};
    for (auto& terms : sumTermLists) {
        std::vector<std::vector<ExprPtr>> next;
        for (auto& combo : combos) {
            for (auto& t : terms) {
                auto c2 = combo;
                c2.push_back(t);
                next.push_back(c2);
            }
        }
        combos = std::move(next);
    }

    std::vector<ExprPtr> outTerms;
    for (auto& combo : combos) {
        std::vector<ExprPtr> f2 = nonAdd;
        for (auto& c : combo) f2.push_back(c);
        outTerms.push_back(Expr::mul(f2)->simplify());
    }
    return Expr::add(outTerms)->simplify();
}

ExprPtr Expr::expand() const {
    switch (kind) {
        case Kind::Number:
        case Kind::Symbol:
            return self();
        case Kind::Add: {
            std::vector<ExprPtr> terms;
            for (auto& a : args) terms.push_back(a->expand());
            return Expr::add(terms)->simplify();
        }
        case Kind::Mul: {
            std::vector<ExprPtr> factors;
            for (auto& a : args) factors.push_back(a->expand());
            return expandMulOfSums(factors);
        }
        case Kind::Pow: {
            ExprPtr base = args[0]->expand();
            ExprPtr exp = args[1]->simplify();
            // expand small positive integer powers of sums: (x+1)^3 -> ...
            if (base->kind == Kind::Add && exp->kind == Kind::Number &&
                exp->value >= 1 && exp->value <= 12 && sameNumber(exp->value, std::round(exp->value))) {
                int n = (int)std::llround(exp->value);
                std::vector<ExprPtr> factors(n, base);
                return expandMulOfSums(factors);
            }
            return Expr::pow(base, exp)->simplify();
        }
        case Kind::Func: {
            std::vector<ExprPtr> a;
            for (auto& t : args) a.push_back(t->expand());
            auto e = std::make_shared<Expr>(Kind::Func);
            e->name = name;
            e->args = a;
            return e->simplify();
        }
        case Kind::Call: {
            std::vector<ExprPtr> a;
            for (auto& t : args) a.push_back(t->expand());
            auto e = std::make_shared<Expr>(Kind::Call);
            e->name = name;
            e->args = a;
            return e;
        }
    }
    return self();
}

// ============================================================================
// substitution
// ============================================================================

ExprPtr Expr::substitute(const std::string& what, ExprPtr with) const {
    switch (kind) {
        case Kind::Number: return self();
        case Kind::Symbol: return name == what ? with : self();
        case Kind::Add: {
            std::vector<ExprPtr> a;
            for (auto& t : args) a.push_back(t->substitute(what, with));
            return Expr::add(a);
        }
        case Kind::Mul: {
            std::vector<ExprPtr> a;
            for (auto& t : args) a.push_back(t->substitute(what, with));
            return Expr::mul(a);
        }
        case Kind::Pow:
            return Expr::pow(args[0]->substitute(what, with), args[1]->substitute(what, with));
        case Kind::Func: {
            std::vector<ExprPtr> a;
            for (auto& t : args) a.push_back(t->substitute(what, with));
            auto e = std::make_shared<Expr>(Kind::Func);
            e->name = name;
            e->args = a;
            return e;
        }
        case Kind::Call: {
            std::vector<ExprPtr> a;
            for (auto& t : args) a.push_back(t->substitute(what, with));
            auto e = std::make_shared<Expr>(Kind::Call);
            e->name = name;
            e->args = a;
            return e;
        }
    }
    return self();
}

ExprPtr Expr::substitute(const ExprPtr& pattern, const ExprPtr& with) const {
    ExprPtr me = self();
    if (me->equalsStruct(pattern)) return with;
    switch (kind) {
        case Kind::Number:
        case Kind::Symbol:
            return me;
        case Kind::Add: {
            std::vector<ExprPtr> a;
            for (auto& t : args) a.push_back(t->substitute(pattern, with));
            return Expr::add(a);
        }
        case Kind::Mul: {
            std::vector<ExprPtr> a;
            for (auto& t : args) a.push_back(t->substitute(pattern, with));
            return Expr::mul(a);
        }
        case Kind::Pow:
            return Expr::pow(args[0]->substitute(pattern, with), args[1]->substitute(pattern, with));
        case Kind::Func: {
            std::vector<ExprPtr> a;
            for (auto& t : args) a.push_back(t->substitute(pattern, with));
            auto e = std::make_shared<Expr>(Kind::Func);
            e->name = name;
            e->args = a;
            return e;
        }
        case Kind::Call: {
            std::vector<ExprPtr> a;
            for (auto& t : args) a.push_back(t->substitute(pattern, with));
            auto e = std::make_shared<Expr>(Kind::Call);
            e->name = name;
            e->args = a;
            return e;
        }
    }
    return me;
}

// ============================================================================
// eval
// ============================================================================

static const std::map<std::string, double>& constants() {
    static const std::map<std::string, double> C = {
        {"pi", M_PI}, {"e", M_E},
    };
    return C;
}

double Expr::eval(const Env& env) const {
    switch (kind) {
        case Kind::Number: return value;
        case Kind::Symbol: {
            auto it = env.find(name);
            if (it != env.end()) return it->second;
            auto cit = constants().find(name);
            if (cit != constants().end()) return cit->second;
            throw EvalError("undefined symbol '" + name + "'");
        }
        case Kind::Add: {
            double s = 0;
            for (auto& a : args) s += a->eval(env);
            return s;
        }
        case Kind::Mul: {
            double p = 1;
            for (auto& a : args) p *= a->eval(env);
            return p;
        }
        case Kind::Pow:
            return std::pow(args[0]->eval(env), args[1]->eval(env));
        case Kind::Func: {
            double a = args[0]->eval(env);
            if (name == "sin") return std::sin(a);
            if (name == "cos") return std::cos(a);
            if (name == "tan") return std::tan(a);
            if (name == "asin") return std::asin(a);
            if (name == "acos") return std::acos(a);
            if (name == "atan") return std::atan(a);
            if (name == "sinh") return std::sinh(a);
            if (name == "cosh") return std::cosh(a);
            if (name == "tanh") return std::tanh(a);
            if (name == "exp") return std::exp(a);
            if (name == "sqrt") {
                if (a < 0) throw EvalError("sqrt of negative number");
                return std::sqrt(a);
            }
            if (name == "abs") return std::abs(a);
            if (name == "floor") return std::floor(a);
            if (name == "ceil") return std::ceil(a);
            if (name == "sign") return (a > 0) - (a < 0);
            if (name == "ln" || name == "log") {
                if (args.size() == 2) {
                    double base = a;
                    double x = args[1]->eval(env);
                    if (x <= 0) throw EvalError("log of non-positive number");
                    return std::log(x) / std::log(base);
                }
                if (a <= 0) throw EvalError("ln of non-positive number");
                return std::log(a);
            }
            throw EvalError("unknown function '" + name + "'");
        }
        case Kind::Call:
            throw EvalError("cannot evaluate call to user function '" + name +
                             "' without a Function registry (use Function::eval instead)");
    }
    throw EvalError("unhandled expr kind");
}

} // namespace math
