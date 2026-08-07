// ============================================================================
// mathlib bridge -- exposes math::Function (symbolic calculus/domain/limits)
// and math::Matrix (linear algebra/eigenvalues) plus the standalone
// math::roots()/math::divisors() helpers as CnR built-in functions.
//
// Design notes:
//  - mathlib's Function/Matrix/Expr types are not CnR Values, so this layer
//    is pure adaptation: CnR strings become parsed math::Expr/Function
//    objects (kept in a name -> Function registry, since CnR has no notion
//    of a symbolic-function value type), CnR Matrix/Tensor Values (an array
//    with `dims`) convert to/from math::Matrix, and math::Complex results
//    become either a plain number (if the imaginary part is negligible) or
//    a small CnR struct-like object {re, im} otherwise.
//  - All new builtins are prefixed (mathX / matX) to avoid colliding with
//    the existing sqrt/pow/... builtin names above.
// ============================================================================

#include "mathlib_flat/mathlib.h"

// ---- Unity build: pull mathlib's implementation files directly into this
// translation unit so a single `g++ CnR.cpp -o CnR` (no -I flag, no extra
// source files on the command line) is enough to build. mathlib's headers
// and .cpp files are kept flattened into mathlib_flat/ alongside CnR.cpp
// specifically so their own unqualified `#include "Expr.h"`-style includes
// resolve without needing an include path.
//
// Safe here because CnR.cpp is the only .cpp in this project that includes
// math_bridge.inc, so each mathlib .cpp is compiled exactly once no matter
// how the project is built. If mathlib is ever linked in as a separate
// library/object elsewhere (e.g. via mathlib/CMakeLists.txt against the
// original mathlib/ directory), do NOT also include this .inc in that same
// binary or you'll get duplicate-symbol link errors -- pick one integration
// method, not both.
#include "mathlib_flat/Expr.cpp"
#include "mathlib_flat/Calculus.cpp"
#include "mathlib_flat/Parser.cpp"
#include "mathlib_flat/Interval.cpp"
#include "mathlib_flat/Numeric.cpp"
#include "mathlib_flat/DomainInference.cpp"
#include "mathlib_flat/Function.cpp"
#include "mathlib_flat/EigenSolver.cpp"
#include "mathlib_flat/LinearAlgebra.cpp"
#include "mathlib_flat/MatrixEigen.cpp"
#include "mathlib_flat/math.cpp"
#include "mathlib_flat/factor.cpp"

// ---- Function registry: CnR has no symbolic-function value type, so
// mathDefine("f(x) = ...") stores the parsed math::Function in a global,
// process-wide registry keyed by name, and every other mathX builtin takes
// that name as its first argument and looks it up here.
std::unordered_map<std::string, math::Function>& mathFunctionRegistry() {
    static std::unordered_map<std::string, math::Function> reg;
    return reg;
}

math::Function& mathLookupFunction(const std::string& name) {
    auto it = mathFunctionRegistry().find(name);
    if(it == mathFunctionRegistry().end())
        throw std::runtime_error("No math function named '" + name + "' -- define it first with mathDefine(\"" + name + "(x) = ...\")");
    return it->second;
}

// ---- Polynomial-coefficient extraction from a mathDefine()'d Function -----
static void mathCollectPolyTerm(const math::ExprPtr& e, const std::string& var,
                                 double coeffFactor, int powFactor,
                                 std::map<int,double>& out) {
    using math::Kind;
    switch(e->kind) {
        case Kind::Number:
            out[powFactor] += coeffFactor * e->value;
            return;
        case Kind::Symbol:
            if(e->name == var) { out[powFactor + 1] += coeffFactor; return; }
            if(e->name == "pi") { out[powFactor] += coeffFactor * M_PI; return; }
            if(e->name == "e")  { out[powFactor] += coeffFactor * M_E; return; }
            throw std::runtime_error("mathRoots(): function body isn't a polynomial in '" + var + "' (found symbol '" + e->name + "')");
        case Kind::Add:
            for(auto& a : e->args) mathCollectPolyTerm(a, var, coeffFactor, powFactor, out);
            return;
        case Kind::Mul: {
            double factor = coeffFactor;
            int pw = powFactor;
            for(auto& a : e->args) {
                if(a->kind == Kind::Number) { factor *= a->value; continue; }
                if(a->kind == Kind::Symbol && a->name == var) { pw += 1; continue; }
                if(a->kind == Kind::Pow && a->args[0]->kind == Kind::Symbol &&
                   a->args[0]->name == var && a->args[1]->kind == Kind::Number) {
                    double ex = a->args[1]->value;
                    if(ex < 0 || ex != std::floor(ex))
                        throw std::runtime_error("mathRoots(): non-polynomial exponent on '" + var + "'");
                    pw += (int)ex;
                    continue;
                }
                throw std::runtime_error("mathRoots(): function body isn't a plain polynomial in '" + var + "'");
            }
            out[pw] += factor;
            return;
        }
        case Kind::Pow: {
            if(e->args[0]->kind == Kind::Symbol && e->args[0]->name == var &&
               e->args[1]->kind == Kind::Number) {
                double ex = e->args[1]->value;
                if(ex < 0 || ex != std::floor(ex))
                    throw std::runtime_error("mathRoots(): non-polynomial exponent on '" + var + "'");
                out[powFactor + (int)ex] += coeffFactor;
                return;
            }
            throw std::runtime_error("mathRoots(): function body isn't a plain polynomial in '" + var + "'");
        }
        default:
            throw std::runtime_error("mathRoots(): function body isn't a plain polynomial (contains a non-algebraic call/func)");
    }
}

std::vector<double> mathPolyCoeffsFromFunction(const math::Function& f) {
    if(f.params.empty())
        throw std::runtime_error("mathRoots(): function '" + f.name + "' has no parameters");
    const std::string& var = f.params[0];
    std::map<int,double> terms;
    mathCollectPolyTerm(f.body->expand(), var, 1.0, 0, terms);
    int maxDeg = 0;
    for(auto& kv : terms) maxDeg = std::max(maxDeg, kv.first);
    std::vector<double> coeffs(maxDeg + 1, 0.0);
    for(auto& kv : terms) coeffs[maxDeg - kv.first] = kv.second;
    return coeffs;
}

bool isMathLibBuiltinName(const std::string& name) {
    static const std::unordered_map<std::string,int> names = {
        {"mathDefine",0}, {"define",0}, {"mathEval",0}, {"mathToString",0},
        {"mathDerivative",0}, {"mathGradient",0}, {"mathIntegral",0}, {"mathDefiniteIntegral",0},
        {"mathTaylorSeries",0}, {"mathDomain",0}, {"mathDomainExact",0}, {"mathImage",0},
        {"mathLimit",0}, {"mathLimitAtInfinity",0},
        {"mathSum",0}, {"mathProduct",0},
        {"mathIsBijective",0}, {"mathInverse",0}, {"mathNumericInverseAt",0},
        {"mathRoots",0}, {"mathRootsImag",0}, {"divisors",0},
        {"matDeterminant",0}, {"matInverse",0}, {"matRank",0}, {"matSolve",0},
        {"matEigenvalues",0}, {"matEigenvaluesImag",0}, {"matTranspose",0}, {"matMultiply",0},
        {"complexEval",0},
    };
    return names.count(name) > 0;
}

// ---- Matrix <-> CnR Value (Value with dims=={rows,cols}) conversion ----

math::Matrix cnrValueToMatrix(const Value& v, const std::string& argLabel) {
    if(!v.isArray || v.dims.size() != 2)
        throw std::runtime_error(argLabel + " must be a 2-D Matrix value, e.g. `var A[2][2] = {{1,2},{3,4}};`");
    int rows = v.dims[0], cols = v.dims[1];
    math::Matrix m(rows, cols);
    for(int r = 0; r < rows; ++r)
        for(int c = 0; c < cols; ++c)
            m(r, c) = v.array[(size_t)r * cols + c];
    return m;
}

Value matrixToCnrValue(const math::Matrix& m) {
    Value v;
    v.isArray = true;
    v.dims = {m.rows(), m.cols()};
    v.array.reserve((size_t)m.rows() * m.cols());
    for(int r = 0; r < m.rows(); ++r)
        for(int c = 0; c < m.cols(); ++c)
            v.array.push_back(m(r, c));
    return v;
}

// A math::Complex with a negligible imaginary part becomes a plain CnR
// number; otherwise it becomes a CnR object {re: ..., im: ...} so callers
// can still inspect both parts (CnR has no native complex type).
Value complexToCnrValue(const math::Complex& c) {
    if(std::fabs(c.imag()) < 1e-9) return makePlainNumber(c.real());
    Value v = Value::makeObject();
    (*v.ex().object)["re"] = makePlainNumber(c.real());
    (*v.ex().object)["im"] = makePlainNumber(c.imag());
    v.ex().isComplexNumber = true; // pretty-prints as "a+bi" -- see valueToDisplayString()
    return v;
}

// ---- Complex-number expression evaluation -------------------------------
//
// mathlib's own Expr::eval() is real (double) only. Rather than threading a
// second, complex-valued evaluator through the whole symbolic engine
// (simplify/derivative/integral/domain-inference all assume real), complex
// arithmetic is supported as a separate, self-contained evaluation mode:
// parse the input the normal way (math::Parser::parseExpression -- the
// grammar/tokenizer doesn't care whether the result will be read as real or
// complex), then walk the resulting Expr tree interpreting the symbol "i"
// as the imaginary unit instead of a free variable. This is enough to
// cover ordinary complex arithmetic/expressions such as "2i * 5" or
// "(5 + 20i) * (sin(pi/2)*i + cos(pi))" -- note "2i" parses for free as
// implicit multiplication (Parser.cpp already turns "2x" into 2*x for any
// trailing identifier, "i" included).
math::Complex evalComplexExpr(const math::ExprPtr& e) {
    using math::Kind;
    using math::Complex;
    switch(e->kind) {
        case Kind::Number:
            return Complex(e->value, 0.0);
        case Kind::Symbol: {
            if(e->name == "i") return Complex(0.0, 1.0);
            if(e->name == "pi") return Complex(M_PI, 0.0);
            if(e->name == "e") return Complex(M_E, 0.0);
            throw std::runtime_error("complexEval(): free symbol '" + e->name +
                "' is not defined -- only numbers, 'i' (imaginary unit), 'pi', and 'e' are supported");
        }
        case Kind::Add: {
            Complex acc(0.0, 0.0);
            for(auto& a : e->args) acc += evalComplexExpr(a);
            return acc;
        }
        case Kind::Mul: {
            Complex acc(1.0, 0.0);
            for(auto& a : e->args) acc *= evalComplexExpr(a);
            return acc;
        }
        case Kind::Pow: {
            Complex base = evalComplexExpr(e->args[0]);
            Complex exp = evalComplexExpr(e->args[1]);
            return std::pow(base, exp);
        }
        case Kind::Func: {
            std::vector<Complex> a;
            a.reserve(e->args.size());
            for(auto& arg : e->args) a.push_back(evalComplexExpr(arg));
            const std::string& fn = e->name;
            if(fn == "sin") return std::sin(a[0]);
            if(fn == "cos") return std::cos(a[0]);
            if(fn == "tan") return std::sin(a[0]) / std::cos(a[0]);
            if(fn == "sinh") return std::sinh(a[0]);
            if(fn == "cosh") return std::cosh(a[0]);
            if(fn == "tanh") return std::tanh(a[0]);
            if(fn == "asin") return std::asin(a[0]);
            if(fn == "acos") return std::acos(a[0]);
            if(fn == "atan") return std::atan(a[0]);
            if(fn == "exp") return std::exp(a[0]);
            if(fn == "ln") return std::log(a[0]);
            if(fn == "log") return a.size() == 2 ? (std::log(a[1]) / std::log(a[0])) // log(base, x)
                                                   : (std::log(a[0]) / std::log(Complex(10.0, 0.0)));
            if(fn == "sqrt") return std::sqrt(a[0]);
            if(fn == "abs") return Complex(std::abs(a[0]), 0.0);
            if(fn == "floor" || fn == "ceil" || fn == "sign") {
                if(std::fabs(a[0].imag()) > 1e-9)
                    throw std::runtime_error("complexEval(): '" + fn + "' is only defined for real inputs");
                double r = a[0].real();
                if(fn == "floor") return Complex(std::floor(r), 0.0);
                if(fn == "ceil") return Complex(std::ceil(r), 0.0);
                return Complex((r > 0) - (r < 0), 0.0);
            }
            throw std::runtime_error("complexEval(): function '" + fn + "' is not supported in complex mode");
        }
        case Kind::Call:
            throw std::runtime_error("complexEval(): calls to user-defined functions (via mathDefine/define) "
                                      "are not supported in complex mode -- inline the expression instead");
    }
    throw std::runtime_error("complexEval(): unhandled expression kind");
}

// math::Set (domain/image) -> its toString(), as a CnR string Value.
Value setToCnrValue(const math::Set& s) {
    return Value::makeString(s.toString());
}

Value limitOutcomeToCnrValue(const math::Function::LimitOutcome& lo) {
    Value v = Value::makeObject();
    auto& obj = *v.ex().object;
    obj["exists"] = Value::makeBool2(lo.exists);
    obj["value"] = makePlainNumber(lo.value);
    obj["isInfinite"] = Value::makeBool2(lo.isInfinite);
    obj["sign"] = makePlainNumber(lo.sign);
    obj["description"] = Value::makeString(lo.description);
    return v;
}

std::string cnrValueToStdString(const Value& v, const std::string& argLabel) {
    if(!v.isString) throw std::runtime_error(argLabel + " must be a string");
    return v.str;
}

// ---- Precision helpers -------------------------------------------------
//
// mathlib's entire evaluation engine (Expr::eval, Numeric.cpp's root
// finding/integration/limits) is built on `double`. There is no arbitrary-
// precision path through sin/cos/exp/ln/sqrt of a general symbolic
// expression -- CnR's own BigDecimal only has an exact sqrt (sqrtNewton());
// everything else (including CnR's own BigFloat sin/cos/exp/log builtins)
// already bottoms out at `long double` precision (~18-19 significant
// digits), by the same design tradeoff documented on callMathBuiltin()
// above. So the honest improvement available here is:
//   1. Route every mathlib argument through `long double` (valueToLongDouble)
//      instead of `double` (valueToApproxDouble), so a BigInt/BigFloat
//      argument doesn't get truncated an extra, unnecessary time on the way
//      in -- it keeps every digit a `long double` can hold rather than only
//      what a `double` can hold.
//   2. When the argument was itself typed as BigInt/BigFloat, return the
//      mathlib result as a BigFloat too (so it composes with further exact
//      arithmetic on the CnR side) instead of silently downgrading it to a
//      plain double.
//   3. If the caller explicitly asked for more precision than a long double
//      can supply (BigFloat(N) with N beyond ~19 significant digits) for an
//      operation that has to evaluate the symbolic expression numerically
//      (as opposed to pure BigInt/BigDecimal arithmetic), say so plainly
//      instead of quietly handing back a value that looks like it has N
//      exact digits but doesn't.
static const int CNR_LONG_DOUBLE_SIG_DIGITS = 19; // honest limit of long double's mantissa

bool valueRequestsBigPrecision(const Value& v) {
    return v.numKind == NumKind::Big || v.numKind == NumKind::BigF;
}

// Throws if `v` is a BigFloat(N) asking for more significant digits than a
// long double can actually deliver through mathlib's double-based evaluator.
void checkPrecisionFeasible(const Value& v, const std::string& fnName) {
    if(v.numKind == NumKind::BigF && v.cex().bigDec && v.cex().bigDec->precision > CNR_LONG_DOUBLE_SIG_DIGITS) {
        throw std::runtime_error(fnName + "(): requested " + std::to_string(v.cex().bigDec->precision) +
            " digits of precision (BigFloat(" + std::to_string(v.cex().bigDec->precision) + ")), but this operation "
            "evaluates the symbolic expression numerically through mathlib's double-precision engine (sin/cos/exp/sqrt/"
            "etc. are not exact in arbitrary precision) -- only about " + std::to_string(CNR_LONG_DOUBLE_SIG_DIGITS) +
            " significant digits are actually meaningful here. Lower the requested precision, or if the function is "
            "a pure polynomial/rational expression, evaluate it directly with CnR's own BigFloat arithmetic instead.");
    }
}

// Converts a mathlib long double result back into a CnR Value: BigFloat if
// any input argument was itself BigInt/BigFloat (so the "exact-looking"
// numeric type propagates through the call), otherwise a plain number --
// unchanged behavior for ordinary Int/Float/plain-double callers.
Value longDoubleResultLike(long double result, const std::vector<Value>& influencingArgs) {
    bool wantsBig = false;
    int precision = CNR_BIGDEC_DEFAULT_SCALE;
    for(auto& a : influencingArgs) {
        if(valueRequestsBigPrecision(a)) {
            wantsBig = true;
            precision = std::min(precision > valuePrecisionOrDefault(a) ? precision : valuePrecisionOrDefault(a), CNR_LONG_DOUBLE_SIG_DIGITS);
        }
    }
    if(wantsBig) return makeBigFloatValue(result, precision);
    return makePlainNumber((double)result);
}

// ---- Exact BigDecimal evaluator for polynomial/rational Expr trees -----
//
// mathlib's own Expr::eval() is double-only. For the common case of a
// polynomial (or rational) function -- no sin/cos/exp/ln/sqrt involved --
// CnR's exact BigDecimal arithmetic (add/sub/mul/div, all exact to the
// working precision) can evaluate the *same* expression tree exactly,
// which is what actually makes an arbitrary-precision definite integral
// meaningful. Throws if the tree contains anything else (Func, Call, or a
// non-integer Pow exponent), since those aren't exact in BigDecimal here
// (sin/cos/exp/ln have no BigDecimal implementation; sqrt does via
// sqrtNewton() but general non-integer powers don't).
BigDecimal evalExprExact(const math::ExprPtr& e, const std::string& var, const BigDecimal& x, int precision) {
    switch(e->kind) {
    case math::Kind::Number:
        return BigDecimal::fromLongDouble((long double)e->value, precision);
    case math::Kind::Symbol:
        if(e->name == var) return x;
        if(e->name == "pi") return cnrConstantAtPrecision(CNR_PI_DIGITS, precision);
        if(e->name == "e") return cnrConstantAtPrecision(CNR_E_DIGITS, precision);
        throw std::runtime_error("free symbol '" + e->name + "' has no exact BigDecimal value");
    case math::Kind::Add: {
        BigDecimal acc = BigDecimal::fromLongDouble(0.0L, precision);
        for(auto& arg : e->args) acc = acc.add(evalExprExact(arg, var, x, precision));
        return acc;
    }
    case math::Kind::Mul: {
        BigDecimal acc = BigDecimal::fromLongDouble(1.0L, precision);
        for(auto& arg : e->args) acc = acc.mul(evalExprExact(arg, var, x, precision));
        return acc;
    }
    case math::Kind::Pow: {
        BigDecimal base = evalExprExact(e->args[0], var, x, precision);
        // The exponent subtree isn't always a bare Number node -- e.g. the
        // parser represents `x^(-2)` as Pow(x, Mul(Number(-1), Number(2))),
        // not Pow(x, Number(-2)). Evaluate the exponent subtree numerically
        // (it must not depend on `var`; mathlib doesn't support symbolic
        // exponents either) and check that it's an integer, rather than
        // requiring one specific tree shape.
        double expVal;
        try {
            expVal = e->args[1]->eval(math::Env{});
        } catch(...) {
            throw std::runtime_error("exponent must not depend on the integration variable to be exact in BigDecimal");
        }
        if(expVal != std::floor(expVal))
            throw std::runtime_error("non-integer exponent is not exact in BigDecimal");
        long long n = (long long)std::llround(expVal);
        bool neg = n < 0;
        n = std::llabs(n);
        BigDecimal r = BigDecimal::fromLongDouble(1.0L, precision);
        for(long long k = 0; k < n; ++k) r = r.mul(base);
        if(neg) r = BigDecimal::fromLongDouble(1.0L, precision).div(r);
        return r;
    }
    case math::Kind::Func:
        throw std::runtime_error("'" + e->name + "(...)' is not exact in BigDecimal (only polynomial/rational expressions are)");
    case math::Kind::Call:
        throw std::runtime_error("calls to other user functions are not exact in BigDecimal");
    }
    throw std::runtime_error("unhandled expression kind in exact BigDecimal evaluator");
}

// Composite Simpson's rule over BigDecimal, exact arithmetic throughout
// (no floating-point rounding error at all -- every add/mul/div is exact
// to `precision` digits). Simpson's rule itself is only *exact* for
// polynomials up to degree 3; for anything else (including rational
// functions like 1/x^2) it's a convergent numeric approximation, same as
// any other numeric integrator, just computed with exact arithmetic at each
// step instead of double rounding. To actually deliver the requested
// number of digits for non-degree-<=3 integrands, this doubles the step
// count and re-integrates until two successive estimates agree to within
// half a unit in the last requested digit (a form of Richardson-style
// convergence check), instead of using one fixed, guessed step count.
BigDecimal simpsonOnce(const math::ExprPtr& body, const std::string& var,
                        const BigDecimal& a, const BigDecimal& b, int steps, int precision) {
    if(steps % 2 != 0) steps += 1;
    BigDecimal h = b.sub(a).div(BigDecimal::fromLongDouble((long double)steps, precision));
    BigDecimal sum = evalExprExact(body, var, a, precision).add(evalExprExact(body, var, b, precision));
    for(int i = 1; i < steps; ++i) {
        BigDecimal xi = a.add(h.mul(BigDecimal::fromLongDouble((long double)i, precision)));
        BigDecimal fx = evalExprExact(body, var, xi, precision);
        BigDecimal coeff = BigDecimal::fromLongDouble((long double)(i % 2 == 0 ? 2 : 4), precision);
        sum = sum.add(fx.mul(coeff));
    }
    return sum.mul(h).div(BigDecimal::fromLongDouble(3.0L, precision));
}

BigDecimal definiteIntegralExact(const math::ExprPtr& body, const std::string& var,
                                   const BigDecimal& a, const BigDecimal& b, int precision) {
    // Threshold for "converged": half a unit in the last requested digit,
    // i.e. 5 * 10^(-precision-1), expressed via a BigDecimal computed at
    // higher working precision so the comparison itself doesn't get
    // swamped by rounding at the target precision.
    int steps = 64;
    static const int kMaxSteps = 2 << 20; // hard ceiling so a pathological/non-convergent integrand can't hang forever
    BigDecimal threshold = BigDecimal::fromString("0." + std::string(precision, '0') + "1", precision + 1);
    BigDecimal prev = simpsonOnce(body, var, a, b, steps, precision);
    while(steps < kMaxSteps) {
        steps *= 2;
        BigDecimal cur = simpsonOnce(body, var, a, b, steps, precision);
        BigDecimal diff = cur.sub(prev);
        // "converged" test: |diff| < 10^-precision, i.e. the change between
        // doubling the step count no longer moves the last requested
        // digit. (roundToPrecision() can't be used for this: BigDecimal's
        // div() always returns a result with scale==precision already, so
        // rounding "to precision" on a division result is a no-op and would
        // never detect convergence -- hence the explicit threshold compare.)
        BigDecimal diffAbs = diff.abs();
        if(BigDecimal::compare(diffAbs, threshold) < 0) {
            return cur;
        }
        prev = cur;
    }
    throw std::runtime_error("did not converge to the requested precision within " + std::to_string(kMaxSteps) +
                              " Simpson subdivisions -- the integrand may be poorly behaved (sharp peaks, near-singularities) "
                              "over this interval");
}


// ---- Exact symbolic antiderivative for polynomial Expr trees ----------
//
// mathlib's own f.integral() can't be reused here: it routes through
// Expr::simplify(), which folds constant powers via plain `std::pow` (e.g.
// `3^(-1)` becomes the double 0.3333333333333333 baked into the tree)
// before this bridge ever sees the result -- see the long comment on
// mathDefiniteIntegral() below for the full story. This is a separate,
// much more limited antiderivative: only Add/Mul/Pow-with-integer-exponent
// of a single variable, using the power rule term by term, entirely in
// BigDecimal (the fractional coefficient 1/(n+1) is a BigDecimal division,
// never a double). Enough to make x^2, x^3, x^-2, 3*x^5 - 2*x + 1, etc.
// exact and instant; anything else (products of two non-constant factors,
// Func/Call nodes) throws so the caller falls back to numeric Simpson.
//
// Represented as a list of (coefficient, integer exponent) monomial terms
// rather than an Expr tree, since that's all evalExprExact-in-reverse
// actually needs and it sidesteps needing a second, BigDecimal-aware Expr
// simplifier.
struct BigMonomial { BigDecimal coeff; long long exp; };

void collectPolynomialTerms(const math::ExprPtr& e, const std::string& var, int precision,
                             const BigDecimal& coeffSoFar, long long expSoFar, std::vector<BigMonomial>& out) {
    switch(e->kind) {
    case math::Kind::Number:
        out.push_back({coeffSoFar.mul(BigDecimal::fromLongDouble((long double)e->value, precision)), expSoFar});
        return;
    case math::Kind::Symbol:
        if(e->name == var) { out.push_back({coeffSoFar, expSoFar + 1}); return; }
        throw std::runtime_error("free symbol '" + e->name + "' in a polynomial term");
    case math::Kind::Add:
        for(auto& arg : e->args) collectPolynomialTerms(arg, var, precision, coeffSoFar, expSoFar, out);
        return;
    case math::Kind::Mul: {
        // Only supported shape: at most one non-constant factor (the rest
        // must be plain numbers) -- same restriction mathlib's own
        // integral() has for products ("integral of a product of multiple
        // non-constant factors is not supported symbolically").
        BigDecimal constAcc = coeffSoFar;
        std::vector<math::ExprPtr> varFactors;
        for(auto& arg : e->args) {
            if(arg->kind == math::Kind::Number) constAcc = constAcc.mul(BigDecimal::fromLongDouble((long double)arg->value, precision));
            else varFactors.push_back(arg);
        }
        if(varFactors.empty()) { out.push_back({constAcc, expSoFar}); return; }
        if(varFactors.size() > 1) throw std::runtime_error("product of multiple non-constant factors is not a polynomial term");
        collectPolynomialTerms(varFactors[0], var, precision, constAcc, expSoFar, out);
        return;
    }
    case math::Kind::Pow: {
        if(!(e->args[0]->kind == math::Kind::Symbol && e->args[0]->name == var))
            throw std::runtime_error("power base is not the integration variable");
        double expVal;
        try { expVal = e->args[1]->eval(math::Env{}); }
        catch(...) { throw std::runtime_error("exponent must not depend on the integration variable"); }
        if(expVal != std::floor(expVal)) throw std::runtime_error("non-integer exponent is not exact in BigDecimal");
        out.push_back({coeffSoFar, expSoFar + (long long)std::llround(expVal)});
        return;
    }
    case math::Kind::Func:
        throw std::runtime_error("'" + e->name + "(...)' is not a polynomial term");
    case math::Kind::Call:
        throw std::runtime_error("call to another user function is not a polynomial term");
    }
    throw std::runtime_error("unhandled expression kind while collecting polynomial terms");
}

// Antiderivative value F(x) = sum of coeff/(exp+1) * x^(exp+1) for each
// term, evaluated directly at `x` (never rebuilt as an Expr tree, so no
// mathlib simplify() pass ever touches it). exp == -1 (i.e. a 1/x term)
// throws, since its antiderivative is ln|x|, transcendental, not exact in
// BigDecimal here.
BigDecimal evalPolynomialAntiderivativeAt(const std::vector<BigMonomial>& terms, const BigDecimal& x, int precision) {
    BigDecimal acc = BigDecimal::fromLongDouble(0.0L, precision);
    BigDecimal one = BigDecimal::fromLongDouble(1.0L, precision);
    for(auto& t : terms) {
        long long newExp = t.exp + 1;
        if(newExp == 0) throw std::runtime_error("term with exponent -1 integrates to ln|x|, not exact in BigDecimal");
        bool negPow = newExp < 0;
        long long absExp = std::llabs(newExp);
        BigDecimal xPow = one;
        for(long long k = 0; k < absExp; ++k) xPow = xPow.mul(x);
        if(negPow) xPow = one.div(xPow);
        BigDecimal term = t.coeff.mul(xPow).div(BigDecimal::fromLongDouble((long double)newExp, precision));
        acc = acc.add(term);
    }
    return acc;
}


Value callMathLibBuiltin(const std::string& name0, std::vector<Value>& args) {
    // "define" is a short alias for "mathDefine" -- same underlying handler,
    // supports single- and multi-variable definitions alike since the
    // parser already accepts an arbitrary parameter list, e.g.
    // define("f(x,y) = x^2 + y^2") is exactly mathDefine("f(x,y) = ...").
    const std::string& name = (name0 == "define") ? std::string("mathDefine") : name0;
    auto need = [&](size_t n) {
        if(args.size() != n)
            throw std::runtime_error("'" + name + "()' expects " + std::to_string(n) +
                                      " argument(s), got " + std::to_string(args.size()));
    };

    // ---- Function definition / evaluation --------------------------------
    if(name == "mathDefine") {
        // mathDefine("f(x) = sqrt(x - 2)") -> defines f, returns its name.
        need(1);
        std::string src = cnrValueToStdString(args[0], "mathDefine()'s argument");
        auto def = math::Parser::parseDefinition(src);
        math::Function f;
        f.name = def.name;
        f.params = def.params;
        f.body = def.body;
        mathFunctionRegistry()[def.name] = f;
        return Value::makeString(def.name);
    }
    if(name == "mathEval") {
        // mathEval("f", 3) -> f(3.0). Two paths, same idea as
        // mathDefiniteIntegral(): try the exact route first, only fall back
        // to mathlib's double-based evaluator (math::Function::operator())
        // when the exact route can't handle this expression.
        //
        // mathEval("f", x, y, ...) -- multivariable: one value per
        // parameter of f, in declaration order (matches how f was defined,
        // e.g. mathDefine("f(x,y) = ...")). Always plain-double here; the
        // BigDecimal-exact path below is single-variable only.
        if(args.size() < 2)
            throw std::runtime_error("'mathEval()' expects at least 2 argument(s), got " + std::to_string(args.size()));
        auto& f = mathLookupFunction(cnrValueToStdString(args[0], "mathEval()'s 1st argument"));
        if(args.size() > 2) {
            std::vector<double> xs;
            xs.reserve(args.size() - 1);
            for(size_t i = 1; i < args.size(); ++i) xs.push_back(valueToApproxDouble(args[i]));
            return makePlainNumber(f.eval(xs));
        }
        if(valueRequestsBigPrecision(args[1])) {
            int precision = std::min(valuePrecisionOrDefault(args[1]), CNR_BIGDEC_MAX_SCALE);
            std::string var = f.params.empty() ? "x" : f.params[0];
            // Path 1: f.body evaluated exactly in BigDecimal (Number/Symbol/
            // Add/Mul/Pow-with-integer-exponent) -- exact at any requested
            // precision, e.g. mathEval("f", (BigFloat(50))3) for a
            // polynomial f is fully exact, not just double-accurate.
            try {
                BigDecimal x = valueToBigDecimal(args[1], precision);
                BigDecimal r = evalExprExact(f.body, var, x, precision);
                return makeBigFloatValue(r);
            } catch(...) {
                // fall through to path 2
            }
            // Path 2: f involves something with no exact BigDecimal
            // evaluator (sin/cos/exp/ln/sqrt/calls to other functions) --
            // fall back to mathlib's double-based evaluator, but only if
            // the requested precision is honestly achievable through it;
            // otherwise this would quietly return a BigFloat(N) with only
            // ~19 meaningful digits and N-19 digits of noise past that.
            checkPrecisionFeasible(args[1], "mathEval");
            double x = valueToApproxDouble(args[1]);
            double r = f(x);
            return longDoubleResultLike((long double)r, {args[1]});
        }
        double x = valueToApproxDouble(args[1]);
        double r = f(x);
        return makePlainNumber(r);
    }
    if(name == "mathToString") {
        need(1);
        auto& f = mathLookupFunction(cnrValueToStdString(args[0], "mathToString()'s argument"));
        return Value::makeString(f.toString());
    }

    // ---- Calculus ----------------------------------------------------------
    if(name == "mathDerivative") {
        // mathDerivative("f")               -> d/d(params[0]) f, order 1
        // mathDerivative("f", order)        -> d^order/d(params[0])^order f
        // mathDerivative("f", "y")          -> partial derivative d/dy f (multivariable)
        // mathDerivative("f", "y", order)   -> d^order/dy^order f
        if(args.empty() || args.size() > 3)
            throw std::runtime_error("'mathDerivative()' expects 1 to 3 argument(s), got " + std::to_string(args.size()));
        auto& f = mathLookupFunction(cnrValueToStdString(args[0], "mathDerivative()'s 1st argument"));
        std::string wrt = f.params.empty() ? "x" : f.params[0];
        int order = 1;
        if(args.size() == 2) {
            if(args[1].isString) wrt = args[1].str;
            else order = (int)valueToApproxDouble(args[1]);
        } else if(args.size() == 3) {
            wrt = cnrValueToStdString(args[1], "mathDerivative()'s 2nd argument (variable name)");
            order = (int)valueToApproxDouble(args[2]);
        }
        return Value::makeString(f.partialDerivative(wrt, order)->toString());
    }
    if(name == "mathGradient") {
        // mathGradient("f") -> object mapping each parameter name to the
        // (stringified) partial derivative of f wrt that parameter, e.g.
        // for f(x,y) = x^2*y + y^3: {x: "2*x*y", y: "x^2 + 3*y^2"}.
        need(1);
        auto& f = mathLookupFunction(cnrValueToStdString(args[0], "mathGradient()'s argument"));
        if(f.params.empty())
            throw std::runtime_error("mathGradient(): function '" + f.name + "' has no parameters");
        Value v = Value::makeObject();
        auto& obj = *v.ex().object;
        for(auto& p : f.params) obj[p] = Value::makeString(f.partialDerivative(p, 1)->toString());
        return v;
    }
    if(name == "mathIntegral") {
        need(1);
        auto& f = mathLookupFunction(cnrValueToStdString(args[0], "mathIntegral()'s argument"));
        try {
            return Value::makeString(f.integral()->toString());
        } catch(const math::SymbolicError& e) {
            throw std::runtime_error(std::string("mathIntegral(): no closed form found (") + e.what() + "); use mathDefiniteIntegral() for a numeric result instead");
        }
    }
    if(name == "mathDefiniteIntegral") {
        // mathDefiniteIntegral("f", a, b). Ordinary Int/Float bounds use
        // mathlib's own adaptive-Simpson numeric integral (double precision,
        // unchanged from before). If either bound is typed BigInt/BigFloat,
        // this instead computes an exact result in BigDecimal arithmetic
        // (never touching mathlib's own f.integral(): that routes through
        // Expr::simplify(), which folds constant powers via plain
        // `std::pow` -- e.g. the exact fraction 1/3 that shows up
        // differentiating x^3 becomes the double 0.3333333333333333 baked
        // into the expression tree itself before this bridge ever sees it,
        // so evaluating that "simplified" tree in BigDecimal just
        // reproduces double rounding error dressed up as extra digits).
        // Two strategies, tried in order:
        //   1. This bridge's own polynomial antiderivative: power rule
        //      term by term, entirely in BigDecimal, evaluated directly at
        //      both bounds and subtracted. Exact and instant for any
        //      polynomial in a single variable (x^2, 3*x^5 - 2*x + 1, ...).
        //   2. Adaptive composite Simpson in BigDecimal (every add/mul/div
        //      exact; only source of error is the discretization itself),
        //      doubling the step count until the result stops changing at
        //      the requested precision. Covers rational functions (e.g.
        //      1/x^2, whose antiderivative -1/x isn't a polynomial term)
        //      and anything else strategy 1 can't handle -- but Simpson's
        //      rule is exact only for polynomials up to degree 3, so for a
        //      genuinely non-polynomial integrand at high precision,
        //      convergence can be slow (steps needed grows roughly with
        //      precision^4).
        // Either way, only handles f.body built from Number/Symbol/Add/
        // Mul/Pow-with-integer-exponent -- no sin/cos/exp/ln/sqrt/other
        // user-function calls, since those have no exact BigDecimal
        // evaluator here; such bodies throw a clear error instead of
        // silently downgrading to a double result that would look like it
        // has the requested number of digits but doesn't -- call this with
        // plain Int/Float bounds to get that double-precision numeric
        // result on purpose.
        need(3);
        auto& f = mathLookupFunction(cnrValueToStdString(args[0], "mathDefiniteIntegral()'s 1st argument"));
        bool wantsBig = valueRequestsBigPrecision(args[1]) || valueRequestsBigPrecision(args[2]);
        if(wantsBig) {
            int precision = -1;
            for(auto& a : {args[1], args[2]}) if(valueRequestsBigPrecision(a)) precision = std::max(precision, valuePrecisionOrDefault(a));
            if(precision < 0) precision = CNR_BIGDEC_DEFAULT_SCALE; // shouldn't happen: wantsBig implies at least one is Big
            precision = std::min(precision, CNR_BIGDEC_MAX_SCALE);
            BigDecimal a = valueToBigDecimal(args[1], precision);
            BigDecimal b = valueToBigDecimal(args[2], precision);
            std::string var = f.params.empty() ? "x" : f.params[0];

            // Strategy 1: this bridge's own polynomial antiderivative
            // (power rule term by term, pure BigDecimal, never touches
            // mathlib's Expr::simplify()) -- exact and instant whenever
            // f.body is a polynomial (or has no exponent-(-1) term).
            try {
                std::vector<BigMonomial> terms;
                collectPolynomialTerms(f.body, var, precision, BigDecimal::fromLongDouble(1.0L, precision), 0, terms);
                BigDecimal atB = evalPolynomialAntiderivativeAt(terms, b, precision);
                BigDecimal atA = evalPolynomialAntiderivativeAt(terms, a, precision);
                return makeBigFloatValue(atB.sub(atA));
            } catch(...) {
                // fall through to strategy 2
            }

            // Strategy 2: adaptive numeric Simpson, exact arithmetic per
            // step -- covers rational functions (e.g. 1/x^2, which strategy
            // 1 can't handle since its antiderivative -1/x isn't a
            // polynomial term) and anything else strategy 1 rejected.
            try {
                // Doubles the step count until successive Simpson estimates
                // agree to the requested precision (or gives up with a
                // clear error) -- no fixed, guessed step count. Runs
                // directly against f.body (the un-simplified expression
                // tree as originally parsed), not f.integral(), so no
                // double ever enters the computation.
                BigDecimal result = definiteIntegralExact(f.body, var, a, b, precision);
                return makeBigFloatValue(result);
            } catch(const std::exception& e) {
                throw std::runtime_error(std::string("mathDefiniteIntegral(): ") + e.what() +
                    " -- falling back to double precision isn't possible while preserving the BigFloat bounds you passed; "
                    "call mathDefiniteIntegral with plain Int/Float bounds instead if an approximate double result is fine");
            }
        }
        double a = valueToApproxDouble(args[1]);
        double b = valueToApproxDouble(args[2]);
        return makePlainNumber(f.definiteIntegral(a, b));
    }
    if(name == "mathTaylorSeries") {
        need(3);
        auto& f = mathLookupFunction(cnrValueToStdString(args[0], "mathTaylorSeries()'s 1st argument"));
        double at = valueToApproxDouble(args[1]);
        int terms = (int)valueToApproxDouble(args[2]);
        return Value::makeString(f.taylorSeries(at, terms)->toString());
    }

    // ---- Domain / image ------------------------------------------------
    if(name == "mathDomain") {
        need(1);
        auto& f = mathLookupFunction(cnrValueToStdString(args[0], "mathDomain()'s argument"));
        return setToCnrValue(f.domain());
    }
    if(name == "mathDomainExact") {
        need(1);
        auto& f = mathLookupFunction(cnrValueToStdString(args[0], "mathDomainExact()'s argument"));
        return Value::makeBool2(f.domainIsExact());
    }
    if(name == "mathImage") {
        need(1);
        auto& f = mathLookupFunction(cnrValueToStdString(args[0], "mathImage()'s argument"));
        return setToCnrValue(f.image());
    }

    // ---- Limits --------------------------------------------------------
    if(name == "mathLimit") {
        // mathLimit("f", at) or mathLimit("f", at, direction)
        if(args.size() != 2 && args.size() != 3)
            throw std::runtime_error("'mathLimit()' expects 2 or 3 argument(s), got " + std::to_string(args.size()));
        auto& f = mathLookupFunction(cnrValueToStdString(args[0], "mathLimit()'s 1st argument"));
        checkPrecisionFeasible(args[1], "mathLimit");
        double at = valueToApproxDouble(args[1]);
        int direction = args.size() == 3 ? (int)valueToApproxDouble(args[2]) : 0;
        auto outcome = f.limit(at, direction);
        Value v = limitOutcomeToCnrValue(outcome);
        (*v.ex().object)["value"] = longDoubleResultLike((long double)outcome.value, {args[1]});
        return v;
    }
    if(name == "mathLimitAtInfinity") {
        need(2);
        auto& f = mathLookupFunction(cnrValueToStdString(args[0], "mathLimitAtInfinity()'s 1st argument"));
        int direction = (int)valueToApproxDouble(args[1]);
        return limitOutcomeToCnrValue(f.limitAtInfinity(direction));
    }

    // ---- Sums / products -------------------------------------------------
    if(name == "mathSum") {
        need(3);
        auto& f = mathLookupFunction(cnrValueToStdString(args[0], "mathSum()'s 1st argument"));
        long long a = (long long)valueToApproxDouble(args[1]);
        long long b = (long long)valueToApproxDouble(args[2]);
        return makePlainNumber(f.sum(a, b));
    }
    if(name == "mathProduct") {
        need(3);
        auto& f = mathLookupFunction(cnrValueToStdString(args[0], "mathProduct()'s 1st argument"));
        long long a = (long long)valueToApproxDouble(args[1]);
        long long b = (long long)valueToApproxDouble(args[2]);
        return makePlainNumber(f.product(a, b));
    }

    // ---- Bijectivity / inverse -------------------------------------------
    if(name == "mathIsBijective") {
        need(1);
        auto& f = mathLookupFunction(cnrValueToStdString(args[0], "mathIsBijective()'s argument"));
        return Value::makeBool2(f.isBijective());
    }
    if(name == "mathInverse") {
        // mathInverse("f") -> defines "f_inverse" (closed form when possible) and returns its name.
        need(1);
        auto& f = mathLookupFunction(cnrValueToStdString(args[0], "mathInverse()'s argument"));
        math::Function inv = f.inverse();
        std::string invName = inv.name.empty() ? (f.name + "_inverse") : inv.name;
        inv.name = invName;
        mathFunctionRegistry()[invName] = inv;
        return Value::makeString(invName);
    }
    if(name == "mathNumericInverseAt") {
        need(2);
        auto& f = mathLookupFunction(cnrValueToStdString(args[0], "mathNumericInverseAt()'s 1st argument"));
        checkPrecisionFeasible(args[1], "mathNumericInverseAt");
        double y = valueToApproxDouble(args[1]);
        double r = f.numericInverseAt(y);
        return longDoubleResultLike((long double)r, {args[1]});
    }

    // ---- Polynomial roots / divisors (standalone helpers) -----------------
    if(name == "mathRoots" || name == "mathRootsImag") {
        // Two calling conventions:
        //   mathRoots(a_n, ..., a1, a0) -- raw coefficients, highest-degree
        //     first, matching the REPL's `roots 1 0 -4` convention.
        //   mathRoots("fnName") -- a single string naming a function
        //     previously registered with mathDefine(); its body is expanded
        //     and read off as polynomial coefficients automatically.
        // Returns a plain array of the real parts; mathRootsImag(...) (same
        // arguments) returns the matching array of imaginary parts (0 for
        // real roots), since CnR arrays can't hold {re,im} objects directly.
        if(args.empty())
            throw std::runtime_error("'" + name + "()' expects at least 1 coefficient, or a single function-name string");
        std::vector<double> coeffs;
        if(args.size() == 1 && args[0].isString) {
            auto& f = mathLookupFunction(args[0].str);
            coeffs = mathPolyCoeffsFromFunction(f);
        } else {
            coeffs.reserve(args.size());
            for(auto& a : args) coeffs.push_back(valueToApproxDouble(a));
        }
        auto rs = math::roots(coeffs);
        Value out;
        out.isArray = true;
        for(auto& c : rs) out.array.push_back(name == "mathRoots" ? c.real() : c.imag());
        return out;
    }
    if(name == "divisors") {
        // Accepts a plain Int, a BigInt (e.g. (BigInt)357686312646216567629137),
        // or anything valueToBigInt() can promote. Trial division runs in
        // exact BigInt arithmetic (no int64 overflow), up to a sqrt(n) trial
        // limit -- which is the real limiting factor for *any* trial-division
        // approach, BigInt or not: sqrt of a 24-digit number is still a
        // 12-digit search space, too slow to run to completion. So this
        // caps the trial limit and throws a clear, honest error rather than
        // silently truncating/overflowing (the old int64-based behavior) or
        // hanging forever.
        need(1);
        BigInt n = valueToBigInt(args[0]);
        n.negative = false;
        if(n.isZero()) { Value out; out.isArray = true; return out; }

        static const long long kTrialLimit = 100000000LL; // 1e8 trial divisions ceiling
        BigInt limit = BigInt::fromInt64(kTrialLimit);
        BigInt limitSq = limit * limit;
        if(BigInt::cmpAbs(n, limitSq) > 0)
            throw std::runtime_error("divisors(): " + n.toString() + " is too large to factor by trial division "
                                      "(would need more than " + std::to_string(kTrialLimit) + " trial divisions) -- "
                                      "this function is exact but not fast enough for numbers with very large prime factors");

        std::vector<BigInt> smallDivisors, largeDivisors; // i, and n/i for i*i<=n
        BigInt i = BigInt::fromInt64(1);
        BigInt one = BigInt::fromInt64(1);
        while(true) {
            BigInt iSq = i * i;
            if(BigInt::cmpAbs(iSq, n) > 0) break;
            BigInt rem;
            BigInt q = BigInt::divModAbs(n, i, rem);
            if(rem.isZero()) {
                smallDivisors.push_back(i);
                if(!(q == i)) largeDivisors.push_back(q);
            }
            i = i + one;
        }
        // Result, in ascending order: -largest .. -1, 1 .. largest (mirrors
        // math::divisors()'s ±i convention). Values that fit exactly in a
        // double keep full precision; astronomically large results (beyond
        // ~2^53) lose exact integer precision once stored in a plain CnR
        // array element -- CnR's array type holds doubles, not BigInts, so
        // that's an inherent limit of returning results this way rather
        // than a bug in the factoring itself.
        std::vector<BigInt> ordered;
        for(auto it = largeDivisors.rbegin(); it != largeDivisors.rend(); ++it) ordered.push_back(*it);
        for(auto& d : smallDivisors) ordered.push_back(d);
        Value out;
        out.isArray = true;
        for(auto& d : ordered) out.array.push_back(-d.toDouble());
        for(auto it = ordered.rbegin(); it != ordered.rend(); ++it) out.array.push_back(it->toDouble());
        std::sort(out.array.begin(), out.array.end());
        return out;
    }

    // ---- Matrix / linear algebra (operates on CnR Matrix values, i.e.
    // arrays with dims=={rows,cols}) -----------------------------------
    if(name == "matDeterminant") {
        need(1);
        math::Matrix m = cnrValueToMatrix(args[0], "matDeterminant()'s argument");
        return makePlainNumber(m.determinant());
    }
    if(name == "matInverse") {
        need(1);
        math::Matrix m = cnrValueToMatrix(args[0], "matInverse()'s argument");
        try {
            return matrixToCnrValue(m.inverse());
        } catch(const math::SingularMatrixError& e) {
            throw std::runtime_error(std::string("matInverse(): ") + e.what());
        }
    }
    if(name == "matRank") {
        need(1);
        math::Matrix m = cnrValueToMatrix(args[0], "matRank()'s argument");
        return makePlainNumber(m.rank());
    }
    if(name == "matSolve") {
        need(2);
        math::Matrix A = cnrValueToMatrix(args[0], "matSolve()'s 1st argument");
        math::Matrix b = cnrValueToMatrix(args[1], "matSolve()'s 2nd argument");
        try {
            return matrixToCnrValue(A.solve(b));
        } catch(const math::MatrixError& e) {
            throw std::runtime_error(std::string("matSolve(): ") + e.what());
        }
    }
    if(name == "matTranspose") {
        need(1);
        math::Matrix m = cnrValueToMatrix(args[0], "matTranspose()'s argument");
        return matrixToCnrValue(m.transpose());
    }
    if(name == "matMultiply") {
        need(2);
        math::Matrix A = cnrValueToMatrix(args[0], "matMultiply()'s 1st argument");
        math::Matrix B = cnrValueToMatrix(args[1], "matMultiply()'s 2nd argument");
        try {
            return matrixToCnrValue(A * B);
        } catch(const math::MatrixError& e) {
            throw std::runtime_error(std::string("matMultiply(): ") + e.what());
        }
    }
    if(name == "matEigenvalues" || name == "matEigenvaluesImag") {
        // Same real/imaginary-parallel-array convention as mathRoots().
        need(1);
        math::Matrix m = cnrValueToMatrix(args[0], name + "()'s argument");
        auto vals = m.eigenvalues();
        Value out;
        out.isArray = true;
        for(auto& c : vals) out.array.push_back(name == "matEigenvalues" ? c.real() : c.imag());
        return out;
    }

    // ---- Complex-number arithmetic ----------------------------------------
    if(name == "complexEval") {
        // complexEval("2i * 5") -> {re: 0, im: 10} (as a CnR object; if the
        // imaginary part is negligible it comes back as a plain number, same
        // convention as mathRoots()/matEigenvalues() -- see complexToCnrValue).
        // "i" is the imaginary unit; "sen" is accepted as an alias for "sin"
        // (canonicalized by the parser), so e.g.
        //   complexEval("(5 + 20i) * (sen(pi/2)*i + cos(pi))")
        // evaluates to -25 - 15i.
        need(1);
        std::string src = cnrValueToStdString(args[0], "complexEval()'s argument");
        math::ExprPtr e = math::Parser::parseExpression(src);
        return complexToCnrValue(evalComplexExpr(e));
    }

    throw std::runtime_error("Unknown built-in math-lib function '" + name + "'");
}
