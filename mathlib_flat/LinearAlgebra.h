#pragma once
//
// LinearAlgebra.h — a small dense-matrix type with the operations this
// library needs: determinant, inverse, linear-system solving, rank,
// transpose/multiply, and eigenvalues/eigenvectors (real or complex,
// via Householder-Hessenberg reduction + Francis implicit double-shift
// QR — the same robust algorithm used internally by math::roots()).
//
// This is not a general-purpose numerical linear algebra library (no
// sparse matrices, no SVD, no iterative solvers): it targets the modest
// matrix sizes that come up when working with polynomials, linear systems
// from a handful of equations, or small linear transformations — the
// kinds of things a "write math functions and reason about them" library
// runs into.

#include <vector>
#include <complex>
#include <string>
#include <stdexcept>
#include <optional>

namespace math {

using Complex = std::complex<double>;

struct MatrixError : std::runtime_error {
    using std::runtime_error::runtime_error;
};

struct SingularMatrixError : MatrixError {
    using MatrixError::MatrixError;
};

struct DimensionError : MatrixError {
    using MatrixError::MatrixError;
};

// A dense matrix of real numbers. Rows and columns are both 0-indexed.
// Stored row-major: m(row, col) is element at that (row, col).
class Matrix {
public:
    Matrix() : m_rows(0), m_cols(0) {}
    Matrix(int rows, int cols, double fill = 0.0);

    static Matrix identity(int n);
    static Matrix fromRows(std::vector<std::vector<double>> rows);
    // column vector from a flat list
    static Matrix columnVector(const std::vector<double>& values);
    static Matrix rowVector(const std::vector<double>& values);

    int rows() const { return m_rows; }
    int cols() const { return m_cols; }
    bool isSquare() const { return m_rows == m_cols; }

    double& operator()(int row, int col);
    double operator()(int row, int col) const;

    std::vector<double>& row(int r) { return m_data[r]; }
    const std::vector<double>& row(int r) const { return m_data[r]; }
    std::vector<double> col(int c) const;

    Matrix operator+(const Matrix& other) const;
    Matrix operator-(const Matrix& other) const;
    Matrix operator*(const Matrix& other) const;
    Matrix operator*(double scalar) const;
    Matrix transpose() const;

    // Gaussian elimination with partial pivoting. Throws DimensionError if
    // not square. Returns 0.0 for a singular matrix (does not throw) since
    // "the determinant is zero" is itself a meaningful, common answer.
    double determinant() const;

    // Row-reduces to find the rank (number of linearly independent
    // rows/columns), via Gaussian elimination with partial pivoting.
    int rank() const;

    // Throws SingularMatrixError if the matrix is singular (or not square).
    Matrix inverse() const;

    // Solves A*x = b via Gaussian elimination with partial pivoting.
    // Throws DimensionError on shape mismatch, SingularMatrixError if A is
    // singular. `b` may be a column vector or a matrix with multiple
    // right-hand-side columns (solves for all of them at once).
    Matrix solve(const Matrix& b) const;

    // Eigenvalues (possibly complex, even for a real matrix) via
    // Householder-Hessenberg reduction + Francis double-shift QR.
    // Throws DimensionError if not square.
    std::vector<Complex> eigenvalues() const;

    // Eigenvalues paired with one eigenvector each, found via inverse
    // iteration seeded at the (approximate) eigenvalue. For real
    // eigenvalues the returned eigenvector is real-valued (imaginary parts
    // negligible); for complex eigenvalues the eigenvector is complex.
    // Best-effort: on numerical difficulty for a particular eigenvalue,
    // that entry's vector may be less precise, but a vector is always
    // returned (normalized to unit length) rather than throwing.
    struct EigenPair {
        Complex value;
        std::vector<Complex> vector;
    };
    std::vector<EigenPair> eigenpairs() const;

    std::string toString() const;

private:
    int m_rows, m_cols;
    std::vector<std::vector<double>> m_data;

    void requireSquare(const char* op) const;
};

} // namespace math
