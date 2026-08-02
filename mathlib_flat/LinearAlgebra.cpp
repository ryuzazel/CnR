#include "LinearAlgebra.h"

#include <cmath>
#include <sstream>
#include <algorithm>

namespace math {

// ============================================================================
// construction
// ============================================================================

Matrix::Matrix(int rows, int cols, double fill)
    : m_rows(rows), m_cols(cols), m_data(rows, std::vector<double>(cols, fill))
{}

Matrix Matrix::identity(int n)
{
    Matrix I(n, n, 0.0);
    for (int i = 0; i < n; ++i) I(i, i) = 1.0;
    return I;
}

Matrix Matrix::fromRows(std::vector<std::vector<double>> rowsData)
{
    if (rowsData.empty()) return Matrix(0, 0);
    int cols = (int)rowsData[0].size();
    for (auto& r : rowsData)
        if ((int)r.size() != cols)
            throw DimensionError("Matrix::fromRows: all rows must have the same length");
    Matrix m((int)rowsData.size(), cols);
    m.m_data = std::move(rowsData);
    return m;
}

Matrix Matrix::columnVector(const std::vector<double>& values)
{
    Matrix m((int)values.size(), 1);
    for (size_t i = 0; i < values.size(); ++i) m(i, 0) = values[i];
    return m;
}

Matrix Matrix::rowVector(const std::vector<double>& values)
{
    Matrix m(1, (int)values.size());
    for (size_t i = 0; i < values.size(); ++i) m(0, i) = values[i];
    return m;
}

// ============================================================================
// element access
// ============================================================================

double& Matrix::operator()(int row, int col)
{
    if (row < 0 || row >= m_rows || col < 0 || col >= m_cols)
        throw DimensionError("Matrix index out of range: (" + std::to_string(row) +
                              ", " + std::to_string(col) + ") for a " + std::to_string(m_rows) +
                              "x" + std::to_string(m_cols) + " matrix");
    return m_data[row][col];
}

double Matrix::operator()(int row, int col) const
{
    if (row < 0 || row >= m_rows || col < 0 || col >= m_cols)
        throw DimensionError("Matrix index out of range: (" + std::to_string(row) +
                              ", " + std::to_string(col) + ") for a " + std::to_string(m_rows) +
                              "x" + std::to_string(m_cols) + " matrix");
    return m_data[row][col];
}

std::vector<double> Matrix::col(int c) const
{
    if (c < 0 || c >= m_cols) throw DimensionError("Matrix::col: column index out of range");
    std::vector<double> out(m_rows);
    for (int r = 0; r < m_rows; ++r) out[r] = m_data[r][c];
    return out;
}

// ============================================================================
// arithmetic
// ============================================================================

Matrix Matrix::operator+(const Matrix& other) const
{
    if (m_rows != other.m_rows || m_cols != other.m_cols)
        throw DimensionError("Matrix addition: shape mismatch (" +
                              std::to_string(m_rows) + "x" + std::to_string(m_cols) + " + " +
                              std::to_string(other.m_rows) + "x" + std::to_string(other.m_cols) + ")");
    Matrix result(m_rows, m_cols);
    for (int i = 0; i < m_rows; ++i)
        for (int j = 0; j < m_cols; ++j)
            result(i, j) = m_data[i][j] + other.m_data[i][j];
    return result;
}

Matrix Matrix::operator-(const Matrix& other) const
{
    if (m_rows != other.m_rows || m_cols != other.m_cols)
        throw DimensionError("Matrix subtraction: shape mismatch (" +
                              std::to_string(m_rows) + "x" + std::to_string(m_cols) + " - " +
                              std::to_string(other.m_rows) + "x" + std::to_string(other.m_cols) + ")");
    Matrix result(m_rows, m_cols);
    for (int i = 0; i < m_rows; ++i)
        for (int j = 0; j < m_cols; ++j)
            result(i, j) = m_data[i][j] - other.m_data[i][j];
    return result;
}

Matrix Matrix::operator*(const Matrix& other) const
{
    if (m_cols != other.m_rows)
        throw DimensionError("Matrix multiplication: shape mismatch (" +
                              std::to_string(m_rows) + "x" + std::to_string(m_cols) + " * " +
                              std::to_string(other.m_rows) + "x" + std::to_string(other.m_cols) + ")");
    Matrix result(m_rows, other.m_cols, 0.0);
    for (int i = 0; i < m_rows; ++i)
        for (int k = 0; k < m_cols; ++k)
        {
            double a = m_data[i][k];
            if (a == 0.0) continue;
            for (int j = 0; j < other.m_cols; ++j)
                result(i, j) += a * other.m_data[k][j];
        }
    return result;
}

Matrix Matrix::operator*(double scalar) const
{
    Matrix result(m_rows, m_cols);
    for (int i = 0; i < m_rows; ++i)
        for (int j = 0; j < m_cols; ++j)
            result(i, j) = m_data[i][j] * scalar;
    return result;
}

Matrix Matrix::transpose() const
{
    Matrix result(m_cols, m_rows);
    for (int i = 0; i < m_rows; ++i)
        for (int j = 0; j < m_cols; ++j)
            result(j, i) = m_data[i][j];
    return result;
}

void Matrix::requireSquare(const char* op) const
{
    if (!isSquare())
        throw DimensionError(std::string(op) + " requires a square matrix, got " +
                              std::to_string(m_rows) + "x" + std::to_string(m_cols));
}

// ============================================================================
// determinant / rank — Gaussian elimination with partial pivoting
// ============================================================================
//
// Both share the same forward-elimination core: reduce to row-echelon form,
// tracking the number of row swaps (for the determinant's sign) and where
// pivots were found (for rank). A pivot smaller than a relative epsilon is
// treated as zero — this is the standard practical tolerance for floating
// point Gaussian elimination, not an exact symbolic zero test.

namespace {
struct EliminationResult {
    std::vector<std::vector<double>> U; // upper-triangular-ish result
    int swaps;
    int pivotCount;
};

EliminationResult forwardEliminate(std::vector<std::vector<double>> A, int rows, int cols)
{
    int swaps = 0;
    int pivotRow = 0;

    for (int col = 0; col < cols && pivotRow < rows; ++col)
    {
        int best = pivotRow;
        double bestVal = std::abs(A[pivotRow][col]);
        for (int r = pivotRow + 1; r < rows; ++r)
        {
            if (std::abs(A[r][col]) > bestVal) { best = r; bestVal = std::abs(A[r][col]); }
        }

        if (bestVal < 1e-12) continue; // no usable pivot in this column

        if (best != pivotRow) { std::swap(A[best], A[pivotRow]); swaps++; }

        for (int r = pivotRow + 1; r < rows; ++r)
        {
            double factor = A[r][col] / A[pivotRow][col];
            if (factor == 0.0) continue;
            for (int c = col; c < cols; ++c) A[r][c] -= factor * A[pivotRow][c];
        }

        pivotRow++;
    }

    return {A, swaps, pivotRow};
}
} // namespace

double Matrix::determinant() const
{
    requireSquare("determinant()");
    if (m_rows == 0) return 1.0; // determinant of the empty matrix, by convention

    auto elim = forwardEliminate(m_data, m_rows, m_cols);
    if (elim.pivotCount < m_rows) return 0.0; // singular

    double det = (elim.swaps % 2 == 0) ? 1.0 : -1.0;
    for (int i = 0; i < m_rows; ++i) det *= elim.U[i][i];
    return det;
}

int Matrix::rank() const
{
    auto elim = forwardEliminate(m_data, m_rows, m_cols);
    return elim.pivotCount;
}

// ============================================================================
// inverse / solve — Gauss-Jordan elimination with partial pivoting
// ============================================================================

Matrix Matrix::solve(const Matrix& b) const
{
    requireSquare("solve()");
    if (b.rows() != m_rows)
        throw DimensionError("Matrix::solve: right-hand side has " + std::to_string(b.rows()) +
                              " rows but the matrix has " + std::to_string(m_rows));

    int n = m_rows;
    int rhsCols = b.cols();

    // augmented matrix [A | b]
    std::vector<std::vector<double>> aug(n, std::vector<double>(n + rhsCols));
    for (int i = 0; i < n; ++i)
    {
        for (int j = 0; j < n; ++j) aug[i][j] = m_data[i][j];
        for (int j = 0; j < rhsCols; ++j) aug[i][n + j] = b(i, j);
    }

    for (int col = 0; col < n; ++col)
    {
        int best = col;
        double bestVal = std::abs(aug[col][col]);
        for (int r = col + 1; r < n; ++r)
            if (std::abs(aug[r][col]) > bestVal) { best = r; bestVal = std::abs(aug[r][col]); }

        if (bestVal < 1e-12)
            throw SingularMatrixError("Matrix::solve: matrix is singular (no unique solution)");

        if (best != col) std::swap(aug[best], aug[col]);

        double pivot = aug[col][col];
        for (int c = col; c < n + rhsCols; ++c) aug[col][c] /= pivot;

        for (int r = 0; r < n; ++r)
        {
            if (r == col) continue;
            double factor = aug[r][col];
            if (factor == 0.0) continue;
            for (int c = col; c < n + rhsCols; ++c) aug[r][c] -= factor * aug[col][c];
        }
    }

    Matrix x(n, rhsCols);
    for (int i = 0; i < n; ++i)
        for (int j = 0; j < rhsCols; ++j)
            x(i, j) = aug[i][n + j];
    return x;
}

Matrix Matrix::inverse() const
{
    requireSquare("inverse()");
    try
    {
        return solve(Matrix::identity(m_rows));
    }
    catch (const SingularMatrixError&)
    {
        throw SingularMatrixError("Matrix::inverse: matrix is singular (determinant is zero)");
    }
}

std::string Matrix::toString() const
{
    std::ostringstream os;
    for (int i = 0; i < m_rows; ++i)
    {
        os << "[";
        for (int j = 0; j < m_cols; ++j)
        {
            if (j) os << ", ";
            os << m_data[i][j];
        }
        os << "]";
        if (i + 1 < m_rows) os << "\n";
    }
    return os.str();
}

} // namespace math
