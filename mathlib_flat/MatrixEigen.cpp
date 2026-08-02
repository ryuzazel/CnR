#include "LinearAlgebra.h"
#include "EigenSolver.h"

#include <cmath>
#include <random>

namespace math {

std::vector<Complex> Matrix::eigenvalues() const
{
    requireSquare("eigenvalues()");
    int n = m_rows;
    if (n == 0) return {};

    detail::RawMatrix A(n, std::vector<Complex>(n));
    for (int i = 0; i < n; ++i)
        for (int j = 0; j < n; ++j)
            A[i][j] = Complex(m_data[i][j], 0.0);

    return detail::eigenvaluesOf(A);
}

// ============================================================================
// eigenvectors via (complex-shifted) inverse iteration
// ============================================================================
//
// Given an approximate eigenvalue lambda, (A - lambda*I) is nearly
// singular in the direction of the corresponding eigenvector, so solving
// (A - (lambda + tiny perturbation)*I) x = random vector repeatedly and
// renormalizing converges rapidly to that eigenvector. This works for both
// real and complex lambda (using complex arithmetic throughout), and is
// the standard practical technique for "I already have the eigenvalues,
// now I want the vectors" — as opposed to computing both simultaneously
// via a full Schur-vector accumulation, which would need every Householder
// and double-shift rotation from eigenvaluesOf() to also be applied to an
// accumulating orthogonal basis. That's a reasonable amount of extra
// bookkeeping for comparatively little benefit at the matrix sizes this
// library targets, so inverse iteration is used instead.

namespace {

using CVec = std::vector<Complex>;
using CMat = std::vector<std::vector<Complex>>;

double cnorm(const CVec& v)
{
    double s = 0;
    for (auto& x : v) s += std::norm(x);
    return std::sqrt(s);
}

void normalizeInPlace(CVec& v)
{
    double n = cnorm(v);
    if (n > 1e-300) for (auto& x : v) x /= n;
}

// Solves (M) x = b for complex M, b via Gaussian elimination with partial
// pivoting (by magnitude), complex arithmetic throughout. Returns false if
// M is (numerically) singular.
bool complexSolve(CMat M, CVec b, CVec& outX)
{
    int n = (int)M.size();
    for (int col = 0; col < n; ++col)
    {
        int best = col;
        double bestVal = std::abs(M[col][col]);
        for (int r = col + 1; r < n; ++r)
            if (std::abs(M[r][col]) > bestVal) { best = r; bestVal = std::abs(M[r][col]); }

        if (bestVal < 1e-14) return false;

        if (best != col) { std::swap(M[best], M[col]); std::swap(b[best], b[col]); }

        Complex pivot = M[col][col];
        for (int c = col; c < n; ++c) M[col][c] /= pivot;
        b[col] /= pivot;

        for (int r = 0; r < n; ++r)
        {
            if (r == col) continue;
            Complex factor = M[r][col];
            if (factor == Complex(0.0)) continue;
            for (int c = col; c < n; ++c) M[r][c] -= factor * M[col][c];
            b[r] -= factor * b[col];
        }
    }
    outX = b;
    return true;
}

CVec inverseIterationEigenvector(const CMat& A, Complex lambda, int n)
{
    // perturb the shift slightly so (A - lambda*I) is nonsingular even when
    // lambda is an exact eigenvalue (which it numerically is, by
    // construction — we want it *barely* nonsingular so the solve still
    // amplifies the eigenvector direction strongly).
    Complex shift = lambda + Complex(1e-10, 1e-10);

    std::mt19937 rng(12345);
    std::uniform_real_distribution<double> dist(-1.0, 1.0);

    CVec v(n);
    for (int i = 0; i < n; ++i) v[i] = Complex(dist(rng), dist(rng));
    normalizeInPlace(v);

    for (int iter = 0; iter < 25; ++iter)
    {
        CMat M(n, CVec(n));
        for (int i = 0; i < n; ++i)
            for (int j = 0; j < n; ++j)
                M[i][j] = A[i][j] - (i == j ? shift : Complex(0.0));

        CVec next;
        if (!complexSolve(M, v, next))
        {
            // shift landed exactly on a singular direction; nudge and retry
            shift += Complex(1e-8, -1e-8);
            continue;
        }
        normalizeInPlace(next);

        // convergence check: next is parallel to v (up to phase)
        Complex dot = 0;
        for (int i = 0; i < n; ++i) dot += std::conj(v[i]) * next[i];
        double alignment = std::abs(dot); // 1.0 if perfectly (anti)parallel

        v = next;
        if (alignment > 1.0 - 1e-10) break;
    }

    // Cosmetic normalization: an eigenvector is only defined up to a
    // (possibly complex) scalar multiple, so the raw iteration result can
    // come out with an arbitrary overall phase — e.g. a real eigenvector
    // like (0, 1) might emerge as (-0.82i, -0.58i), which is correct but
    // unnecessarily confusing to read. Two adjustments:
    //   1. if lambda is (numerically) real, rotate the whole vector by the
    //      phase of its largest-magnitude component so that component
    //      becomes purely real — this makes real eigenvalues produce
    //      real-valued eigenvectors, as expected.
    //   2. flip sign so that largest component is positive, for a
    //      deterministic, human-friendly orientation.
    if (std::abs(lambda.imag()) < 1e-8)
    {
        int maxIdx = 0;
        double maxMag = 0;
        for (int i = 0; i < n; ++i)
            if (std::abs(v[i]) > maxMag) { maxMag = std::abs(v[i]); maxIdx = i; }

        if (maxMag > 1e-300)
        {
            Complex phase = v[maxIdx] / std::abs(v[maxIdx]);
            for (auto& x : v) x /= phase;

            if (v[maxIdx].real() < 0)
                for (auto& x : v) x = -x;

            // clean up floating point noise in the imaginary part now that
            // we've rotated onto the real axis
            for (auto& x : v) x = Complex(x.real(), std::abs(x.imag()) < 1e-9 ? 0.0 : x.imag());
        }
    }

    return v;
}

} // namespace

std::vector<Matrix::EigenPair> Matrix::eigenpairs() const
{
    requireSquare("eigenpairs()");
    int n = m_rows;

    auto vals = eigenvalues();

    CMat A(n, CVec(n));
    for (int i = 0; i < n; ++i)
        for (int j = 0; j < n; ++j)
            A[i][j] = Complex(m_data[i][j], 0.0);

    std::vector<EigenPair> result;
    result.reserve(vals.size());
    for (auto& lambda : vals)
    {
        CVec vec = inverseIterationEigenvector(A, lambda, n);
        result.push_back({lambda, vec});
    }
    return result;
}

} // namespace math
