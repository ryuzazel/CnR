#include "EigenSolver.h"

#include <cmath>
#include <algorithm>

namespace math::detail {

constexpr double EPS = 1e-12;

// --------------------------------------------------
// Reduce to upper Hessenberg form via Householder reflections
// --------------------------------------------------
//
// Zeroes everything below the first subdiagonal via orthogonal similarity
// transforms, which is the prerequisite for the subdiagonal-magnitude
// deflation test used by the QR iteration below to be meaningful.

static RawMatrix toUpperHessenberg(RawMatrix A)
{
    int n = (int)A.size();
    for (int k = 0; k < n - 2; ++k)
    {
        double normX = 0;
        for (int i = k + 1; i < n; ++i) normX += std::norm(A[i][k]);
        normX = std::sqrt(normX);
        if (normX < EPS) continue;

        Complex alpha = A[k + 1][k];
        double alphaAbs = std::abs(alpha);
        Complex sign = alphaAbs < EPS ? Complex(1.0) : alpha / alphaAbs;
        Complex v0 = alpha + sign * normX;

        std::vector<Complex> v(n, Complex(0.0));
        v[k + 1] = v0;
        for (int i = k + 2; i < n; ++i) v[i] = A[i][k];

        double vnorm = 0;
        for (int i = k + 1; i < n; ++i) vnorm += std::norm(v[i]);
        vnorm = std::sqrt(vnorm);
        if (vnorm < EPS) continue;
        for (int i = k + 1; i < n; ++i) v[i] /= vnorm;

        // A <- (I - 2vv*) A   (apply from the left)
        for (int j = 0; j < n; ++j)
        {
            Complex dot = 0;
            for (int i = k + 1; i < n; ++i) dot += std::conj(v[i]) * A[i][j];
            for (int i = k + 1; i < n; ++i) A[i][j] -= Complex(2.0) * v[i] * dot;
        }
        // A <- A (I - 2vv*)   (apply from the right, to keep it a similarity transform)
        for (int i = 0; i < n; ++i)
        {
            Complex dot = 0;
            for (int j = k + 1; j < n; ++j) dot += A[i][j] * std::conj(v[j]);
            for (int j = k + 1; j < n; ++j) A[i][j] -= Complex(2.0) * dot * v[j];
        }
    }
    return A;
}

// --------------------------------------------------
// QR eigen solver — Francis implicit double-shift QR
// --------------------------------------------------
//
// A single real shift cannot make progress whenever the trailing 2x2
// block's eigenvalues are complex-conjugates (or otherwise far from the
// shift target) — the iteration just cycles forever. The Francis implicit
// double-shift uses the two eigenvalues of the trailing 2x2 block (a
// complex-conjugate pair, when relevant) as a *pair* of shifts applied
// together via Householder reflections chased down the subdiagonal,
// staying in real arithmetic throughout while still making genuine
// progress on complex-conjugate pairs.

static void doubleShiftStep(std::vector<std::vector<double>>& H, int p, int q)
{
    // active block is H[q..p][q..p] (inclusive), p > q, p-q >= 1
    int n = (int)H.size();

    double s = H[p-1][p-1] + H[p][p];
    double t = H[p-1][p-1] * H[p][p] - H[p-1][p] * H[p][p-1];

    double x = H[q][q] * H[q][q] + H[q][q+1] * H[q+1][q] - s * H[q][q] + t;
    double y = H[q+1][q] * (H[q][q] + H[q+1][q+1] - s);
    double z = (q + 2 <= p) ? H[q+2][q+1] * H[q+1][q] : 0.0;

    for (int k = q; k <= p - 1; ++k)
    {
        int rows = std::min(3, p - k + 1);
        double v[3] = {x, y, z};
        double normV = 0;
        for (int i = 0; i < rows; ++i) normV += v[i] * v[i];
        normV = std::sqrt(normV);
        if (normV > EPS)
        {
            double alpha = (v[0] >= 0) ? -normV : normV;
            double v0 = v[0] - alpha;
            double vNorm2 = v0 * v0;
            for (int i = 1; i < rows; ++i) vNorm2 += v[i] * v[i];
            if (vNorm2 > EPS * EPS)
            {
                double w[3] = {v0, rows > 1 ? v[1] : 0.0, rows > 2 ? v[2] : 0.0};
                double wnorm = std::sqrt(vNorm2);
                for (int i = 0; i < rows; ++i) w[i] /= wnorm;

                int rowStart = k;
                for (int col = std::max(0, k - 1); col < n; ++col)
                {
                    double dot = 0;
                    for (int i = 0; i < rows; ++i) dot += w[i] * H[rowStart + i][col];
                    for (int i = 0; i < rows; ++i) H[rowStart + i][col] -= 2.0 * w[i] * dot;
                }
                int colEnd = std::min(n - 1, k + 3);
                for (int row = 0; row <= colEnd; ++row)
                {
                    double dot = 0;
                    for (int i = 0; i < rows; ++i) dot += H[row][rowStart + i] * w[i];
                    for (int i = 0; i < rows; ++i) H[row][rowStart + i] -= 2.0 * dot * w[i];
                }
            }
        }

        if (k < p - 1)
        {
            x = H[k+1][k];
            y = H[k+2][k];
            z = (k + 3 <= p) ? H[k+3][k] : 0.0;
        }
    }
}

std::vector<Complex> eigenvaluesOf(RawMatrix Ain)
{
    int n = (int)Ain.size();
    if (n == 0) return {};

    RawMatrix Ac = toUpperHessenberg(std::move(Ain));

    // From here on we work entirely in real double arithmetic: any
    // imaginary parts introduced are pure floating-point noise from the
    // Complex type when the input matrix is real, negligible in magnitude.
    std::vector<std::vector<double>> H(n, std::vector<double>(n));
    for (int i = 0; i < n; ++i)
        for (int j = 0; j < n; ++j)
            H[i][j] = Ac[i][j].real();

    std::vector<Complex> result;
    int p = n - 1;
    int totalIters = 0;
    int maxIters = 500 * n + 2000;

    while (p > 0 && totalIters < maxIters)
    {
        int q = 0;
        for (int i = p; i >= 1; --i)
        {
            double scale = std::abs(H[i-1][i-1]) + std::abs(H[i][i]);
            if (scale < EPS) scale = 1.0;
            if (std::abs(H[i][i-1]) < 1e-12 * scale)
            {
                q = i;
                break;
            }
        }

        if (p - q == 0)
        {
            result.push_back(Complex(H[p][p], 0.0));
            p -= 1;
            continue;
        }

        if (p - q == 1)
        {
            double a = H[q][q], b = H[q][p], c = H[p][q], d = H[p][p];
            double tr = a + d, det = a * d - b * c;
            double disc = tr * tr - 4 * det;
            if (disc >= 0)
            {
                double sq = std::sqrt(disc);
                result.push_back(Complex((tr + sq) / 2.0, 0.0));
                result.push_back(Complex((tr - sq) / 2.0, 0.0));
            }
            else
            {
                double sq = std::sqrt(-disc);
                result.push_back(Complex(tr / 2.0, sq / 2.0));
                result.push_back(Complex(tr / 2.0, -sq / 2.0));
            }
            p -= 2;
            continue;
        }

        doubleShiftStep(H, p, q);
        totalIters++;
    }

    if (p == 0) result.push_back(Complex(H[0][0], 0.0));

    return result;
}

} // namespace math::detail
