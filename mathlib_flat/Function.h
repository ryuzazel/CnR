#pragma once
//
// Function.h — the main user-facing object: a named function f(x) = body,
// with domain/image (auto-inferred or overridden), calculus operations,
// series/limits/sums, composition/inverse, and a small rule engine for
// stating extra facts ("f is bijective", "f(g(x)) = f(x) - 1", ...).

#include "Expr.h"
#include "Interval.h"
#include "DomainInference.h"
#include <string>
#include <vector>
#include <map>
#include <optional>
#include <memory>
#include <functional>

namespace math {

using RealFnLike = std::function<double(double)>;

// A rewrite rule: whenever the pattern (built from this function's params
// and possibly other registered functions) matches, it can be replaced
// with the given replacement. Example:
//   f(g(x)) = f(x) - 1
// is stored as pattern = call("f", {call("g", {x})}), replacement = f(x)-1.
struct Rule {
    std::string description;   // human-readable, e.g. "f(g(x)) = f(x) - 1"
    ExprPtr pattern;           // left-hand side, as an Expr (may contain Call nodes)
    ExprPtr replacement;       // right-hand side
};

enum class Property {
    Bijective,
    Injective,   // one-to-one
    Surjective,  // onto (relative to a stated codomain, usually the image itself)
    Even,        // f(-x) = f(x)
    Odd,         // f(-x) = -f(x)
    Periodic,
    Increasing,
    Decreasing,
};

struct PropertyFact {
    Property prop;
    std::optional<double> period; // used for Periodic
    std::string note;             // e.g. how it was established / any caveat
};

class Registry; // fwd decl, defined in Function.cpp translation unit's header below

class Function {
public:
    std::string name;
    std::vector<std::string> params; // usually just {"x"}
    ExprPtr body;

    // Domain/Image: if not explicitly set, accessed via domain()/image()
    // which lazily auto-infer (and cache) from the body.
    Function& setDomain(Set d);
    Function& setDomain(const std::string& intervalExpr); // e.g. "[0, 10)" or "R"
    Function& setImage(Set i);
    Function& setImage(const std::string& intervalExpr);

    const Set& domain() const;
    const Set& image() const;   // auto-inference for image is approximate (numeric sampling)
    bool domainIsExact() const;

    // ---- calculus (single-variable, wrt the primary param) -----------
    ExprPtr derivative(int order = 1) const;
    // ---- calculus (multivariable) -------------------------------------
    // Partial derivative wrt any named variable (not just params[0]), e.g.
    // for f(x,y) = x^2*y + y^3, partialDerivative("y") gives x^2 + 3*y^2.
    // order > 1 repeats the same partial (d^order f / d(wrt)^order), not a
    // mixed partial -- combine two calls for mixed partials, e.g.
    // f.partialDerivative("x") differentiated again wrt "y" by hand via
    // Expr::derivative() on the resulting body if a mixed partial is needed.
    ExprPtr partialDerivative(const std::string& wrt, int order = 1) const;
    // Gradient: partial derivative wrt each parameter, in declaration order.
    // Requires at least one parameter.
    std::vector<ExprPtr> gradient() const;
    // Symbolic antiderivative (throws SymbolicError if not found in closed form).
    ExprPtr integral() const;
    // Definite integral, numeric (falls back automatically; always works
    // as long as f is defined and finite on [a,b]).
    double definiteIntegral(double a, double b) const;

    // Taylor/Maclaurin series around `at`, truncated to `terms` terms
    // (terms=n means degree up to x^(n-1) beyond the constant, i.e. n total
    // terms including the constant one).
    ExprPtr taylorSeries(double at, int terms) const;

    // One-sided or two-sided limit as x -> `at` (at may be +/- infinity).
    struct LimitOutcome {
        bool exists;
        double value;
        bool isInfinite;
        double sign; // meaningful only if isInfinite
        std::string description; // human-readable summary
    };
    LimitOutcome limit(double at, int direction = 0) const; // direction: -1 left, 0 both, +1 right
    LimitOutcome limitAtInfinity(int direction) const;      // direction: -1 -> -inf, +1 -> +inf

    // Sum_{k=a}^{b} f(k), Product_{k=a}^{b} f(k). `a`,`b` inclusive integers.
    double sum(long long a, long long b) const;
    double product(long long a, long long b) const;

    // ---- evaluation -----------------------------------------------------
    double operator()(double x) const;
    double eval(const std::vector<double>& args) const;

    // ---- composition / algebra -------------------------------------
    // returns a NEW Function representing this(other(x))
    Function composeWith(const Function& inner) const;
    Function plus(const Function& other) const;
    Function minus(const Function& other) const;
    Function times(const Function& other) const;

    // ---- rules ------------------------------------------------------
    Function& addRule(const std::string& description, const std::string& patternExpr, const std::string& replacementExpr);
    Function& addRule(Rule r);
    const std::vector<Rule>& rules() const { return m_rules; }

    // Applies registered rules (of this function and any function referenced
    // in `e`, if a Registry is supplied) to rewrite `e`. Best-effort, single
    // pass unless `repeat` is true (repeats until no rule applies or a cap
    // is hit, to avoid infinite loops on badly-specified rule sets).
    ExprPtr applyRules(const ExprPtr& e, bool repeat = true, int maxPasses = 25) const;

    // ---- properties ---------------------------------------------------
    Function& assertProperty(Property p, const std::string& note = "");
    Function& assertPeriodic(double period, const std::string& note = "");
    bool hasProperty(Property p) const;
    std::optional<double> period() const;

    // Checks bijectivity numerically over the (possibly restricted) domain:
    // strictly monotonic (derivative doesn't change sign, sampled) implies
    // injective; combined with image() giving the codomain, that's bijective
    // onto its image. Also honors an explicit assertProperty(Bijective) if
    // the user has stated it (e.g. because they know something the sampler
    // can't see).
    bool isBijective(int samples = 200) const;
    bool isMonotonic(bool& increasing, int samples = 200) const;

    // Symbolic/numeric inverse. Requires (or checks) bijectivity. For simple
    // symbolic forms (linear, a*x+b; power x^n with odd n or restricted
    // domain; a*e^x+b; a*ln(x)+b) returns a closed form. Otherwise returns
    // a Function that evaluates the inverse numerically via bisection,
    // which still supports eval/plot/etc. Throws if f is not bijective.
    Function inverse() const;

    // Numeric inverse evaluation at a single point: solves f(x) = y for x
    // via bisection over the domain, without requiring a closed form.
    // Requires bijectivity (checked the same way as inverse()).
    double numericInverseAt(double y) const;

    std::string toString() const;

private:
    mutable std::optional<Set> m_domain;
    mutable bool m_domainExact = true;
    mutable std::optional<Set> m_image;

    std::vector<Rule> m_rules;
    std::vector<PropertyFact> m_properties;

    void ensureDomain() const;
    void ensureImage() const;

    RealFnLike evalFn() const; // internal helper, defined in .cpp
};

} // namespace math
