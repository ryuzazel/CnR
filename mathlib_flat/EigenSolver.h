#pragma once
//
// EigenSolver.h — internal shared implementation of the eigenvalue
// algorithm used by both math::roots() (on companion matrices) and
// Matrix::eigenvalues()/eigenpairs() (on general square matrices).
//
// Not part of the public API surface directly (LinearAlgebra.h and math.h
// expose friendlier entry points) — this header just avoids duplicating
// the Householder-Hessenberg reduction and Francis implicit double-shift
// QR implementation in two translation units.

#include <vector>
#include <complex>

namespace math::detail {

using Complex = std::complex<double>;
using RawMatrix = std::vector<std::vector<Complex>>;

// Computes all eigenvalues of a general (possibly non-symmetric) real or
// complex square matrix, given as a row-major vector of vectors. Internally
// reduces to upper Hessenberg form via Householder reflections, then runs
// Francis implicit double-shift QR with 1x1/2x2 deflation — this handles
// real eigenvalue pairs of equal magnitude and complex-conjugate pairs
// correctly (a plain/single-shift QR iteration can cycle forever on both).
//
// The implementation assumes the *effective* matrix is real (companion
// matrices of real polynomials, and any Matrix built from doubles, are);
// any imaginary parts are treated as negligible floating-point noise.
std::vector<Complex> eigenvaluesOf(RawMatrix A);

} // namespace math::detail
