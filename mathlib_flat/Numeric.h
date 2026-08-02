#pragma once
//
// Numeric.h — numeric fallbacks used when a symbolic result isn't available
// or isn't needed: root finding (for domain boundaries / equation solving),
// numeric derivative, numeric definite integral, and limit approximation.

#include <functional>
#include <vector>
#include <optional>

namespace math {

using RealFn = std::function<double(double)>;

// Scans [lo, hi] in `steps` subdivisions looking for sign changes, then
// refines each one via bisection. Returns all roots found (best-effort:
// may miss roots that don't cross zero, e.g. tangent points).
std::vector<double> findRootsScan(const RealFn& f, double lo, double hi, int steps = 2000);

// Bisection on a single bracket known to contain a sign change.
std::optional<double> bisect(const RealFn& f, double lo, double hi, int maxIter = 100, double tol = 1e-10);

// Newton's method from a starting guess (falls back to nullopt on divergence).
std::optional<double> newton(const RealFn& f, const RealFn& fprime, double x0, int maxIter = 50, double tol = 1e-10);

double numericDerivative(const RealFn& f, double x, double h = 1e-6);

// Adaptive Simpson's rule for a definite integral.
double numericIntegral(const RealFn& f, double a, double b, double tol = 1e-9);

// One/two-sided numeric limit approximation via Richardson-style shrinking step.
struct LimitResult {
    bool exists;
    double value;
    bool isInfinite;
    double signedInfinity; // +1 or -1 if isInfinite
};

LimitResult numericLimit(const RealFn& f, double at, int direction /* -1 left, 0 both, +1 right */);
LimitResult numericLimitAtInfinity(const RealFn& f, int direction /* -1 for -inf, +1 for +inf */);

} // namespace math
