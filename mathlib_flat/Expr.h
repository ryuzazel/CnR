#pragma once
//
// Expr.h — symbolic expression tree.
//
// A small computer-algebra core: build expressions, simplify them,
// differentiate / integrate them symbolically, evaluate them numerically,
// and pretty-print them back to a string.
//
// Everything is immutable: every operation returns a NEW ExprPtr rather
// than mutating in place, which keeps the rest of the library (rules,
// composition, series expansion, ...) simple to reason about.

#include <memory>
#include <string>
#include <vector>
#include <map>
#include <optional>
#include <stdexcept>

namespace math {

class Expr;
using ExprPtr = std::shared_ptr<const Expr>;

enum class Kind {
    Number,     // 3.14
    Symbol,     // x, y, pi, e ...
    Add,        // n-ary sum
    Mul,        // n-ary product
    Pow,        // base ^ exponent
    Func,       // named unary/binary function: sin(x), log(b, x), ...
    Call,       // call to a user-defined Function by name: f(x), f(g(x))
};

// Evaluation environment: symbol -> value.
using Env = std::map<std::string, double>;

// Thrown when eval() hits something it cannot resolve numerically
// (undefined symbol, out of domain, user function not registered, ...).
struct EvalError : std::runtime_error {
    using std::runtime_error::runtime_error;
};

// Thrown when a symbolic step (derivative/integral) cannot be completed
// in closed form for the requested expression.
struct SymbolicError : std::runtime_error {
    using std::runtime_error::runtime_error;
};

class Expr : public std::enable_shared_from_this<Expr> {
public:
    Kind kind;

    // Number
    double value = 0.0;

    // Symbol
    std::string name;

    // Add / Mul: operands. Pow: {base, exponent}. Func: {arg} or {arg1, arg2}.
    // Call: {arguments...} with `name` = function name.
    std::vector<ExprPtr> args;

    Expr(Kind k) : kind(k) {}

    // ---- construction helpers -------------------------------------------
    static ExprPtr num(double v);
    static ExprPtr sym(const std::string& n);
    static ExprPtr add(std::vector<ExprPtr> xs);
    static ExprPtr add(ExprPtr a, ExprPtr b);
    static ExprPtr sub(ExprPtr a, ExprPtr b);
    static ExprPtr neg(ExprPtr a);
    static ExprPtr mul(std::vector<ExprPtr> xs);
    static ExprPtr mul(ExprPtr a, ExprPtr b);
    static ExprPtr div(ExprPtr a, ExprPtr b);
    static ExprPtr pow(ExprPtr base, ExprPtr exp);
    static ExprPtr func(const std::string& fname, ExprPtr arg);
    static ExprPtr func(const std::string& fname, ExprPtr a, ExprPtr b);
    static ExprPtr call(const std::string& fname, std::vector<ExprPtr> callArgs);

    // ---- queries ----------------------------------------------------------
    bool isNumber() const { return kind == Kind::Number; }
    bool isNumber(double v) const;
    bool isSymbol(const std::string& n) const { return kind == Kind::Symbol && name == n; }
    bool isZero() const { return isNumber(0.0); }
    bool isOne() const { return isNumber(1.0); }

    // does this subtree mention the symbol `v` at all?
    bool contains(const std::string& v) const;

    // structural equality (after simplification this is a decent proxy for
    // mathematical equality on "nice" expressions; it is NOT a full CAS
    // equivalence check).
    bool equalsStruct(const ExprPtr& other) const;

    // ---- transforms ---------------------------------------------------
    ExprPtr simplify() const;
    // Full polynomial expansion: (a+b)*(c+d) -> a*c + a*d + b*c + b*d, recursively.
    // Slower than simplify() and can blow up for high-degree products, so it's
    // opt-in rather than automatic.
    ExprPtr expand() const;
    ExprPtr derivative(const std::string& wrt = "x") const;
    // Symbolic antiderivative. Throws SymbolicError if none is found.
    ExprPtr integral(const std::string& wrt = "x") const;

    // Substitute `what` -> `with` (structural symbol substitution).
    ExprPtr substitute(const std::string& what, ExprPtr with) const;
    // Substitute an entire subexpression `pattern` -> `with` when it
    // structurally matches (after simplification). Best-effort.
    ExprPtr substitute(const ExprPtr& pattern, const ExprPtr& with) const;

    double eval(const Env& env) const;
    double eval(double x, const std::string& var = "x") const {
        return eval(Env{{var, x}});
    }

    std::string toString() const;

    // shared_from_this wrapper as ExprPtr
    ExprPtr self() const { return shared_from_this(); }
};

// convenience free functions mirroring common math notation
ExprPtr operator+(ExprPtr a, ExprPtr b);
ExprPtr operator-(ExprPtr a, ExprPtr b);
ExprPtr operator*(ExprPtr a, ExprPtr b);
ExprPtr operator/(ExprPtr a, ExprPtr b);
ExprPtr operator-(ExprPtr a);

} // namespace math
