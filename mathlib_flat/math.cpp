#include "math.h"
#include "EigenSolver.h"

#include <vector>
#include <complex>
#include <cmath>

namespace math
{

using Complex = std::complex<double>;
using RawMatrix = std::vector<std::vector<Complex>>;

// --------------------------------------------------
// Companion matrix
// --------------------------------------------------
//
// For coef = {c0, c1, ..., cn} representing c0*x^n + c1*x^(n-1) + ... + cn,
// builds the standard companion matrix whose eigenvalues are exactly the
// roots of that polynomial.

static RawMatrix companion(const std::vector<double>& c)
{
    int n = (int)c.size() - 1;
    RawMatrix C(n, std::vector<Complex>(n));

    double a = c[0];
    for (int i = 0; i < n; ++i) C[0][i] = -c[i + 1] / a;
    for (int i = 1; i < n; ++i) C[i][i - 1] = 1.0;

    return C;
}

// --------------------------------------------------
// PUBLIC ROOT FUNCTION
// --------------------------------------------------

std::vector<Complex> roots(const std::vector<double>& coef)
{
    if (coef.size() < 2) return {};

    RawMatrix C = companion(coef);

    // detail::eigenvaluesOf() reduces to upper Hessenberg form internally
    // and runs Francis double-shift QR, which correctly handles real pairs
    // of equal magnitude and complex-conjugate pairs (both of which a
    // naive single-shift QR iteration cycles on forever).
    return detail::eigenvaluesOf(C);
}

}
