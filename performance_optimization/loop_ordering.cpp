
#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstddef>
#include <iomanip>
#include <iostream>
#include <random>
#include <stdexcept>
#include <string>
#include <vector>

class MatrixOptimization {
public:
    using Matrix = std::vector<std::vector<double>>;

    // Reference matrix multiplication: i-j-k
    static void matrixMultiplyBasic(
        const Matrix& A, const Matrix& B, Matrix& C)
    {
        const std::size_t n = A.size();

        for (std::size_t i = 0; i < n; ++i) {
            for (std::size_t j = 0; j < n; ++j) {
                C[i][j] = 0.0;

                for (std::size_t k = 0; k < n; ++k) {
                    C[i][j] += A[i][k] * B[k][j];
                }
            }
        }
    }

    // Cache-optimized matrix multiplication: i-k-j
    static void matrixMultiplyOptimized(
        const Matrix& A, const Matrix& B, Matrix& C)
    {
        const std::size_t n = A.size();

        for (auto& row : C) {
            std::fill(row.begin(), row.end(), 0.0);
        }

        for (std::size_t i = 0; i < n; ++i) {
            for (std::size_t k = 0; k < n; ++k) {
                const double aik = A[i][k];

                for (std::size_t j = 0; j < n; ++j) {
                    C[i][j] += aik * B[k][j];
                }
            }
        }
    }

    // Basic dot product
    static double dotProductBasic(
        const std::vector<double>& a,
        const std::vector<double>& b)
    {
        if (a.size() != b.size()) {
            throw std::invalid_argument("Vectors must have same size");
        }

        double result = 0.0;

        for (std::size_t i = 0; i < a.size(); ++i) {
            result += a[i] * b[i];
        }

        return result;
    }

    // Dot product with loop unrolling by 4
    static double dotProductUnrolled(
        const std::vector<double>& a,
        const std::vector<double>& b)
    {
        if (a.size() != b.size()) {
            throw std::invalid_argument("Vectors must have same size");
        }

        const std::size_t n = a.size();
        std::size_t i = 0;

        double sum0 = 0.0;
        double sum1 = 0.0;
        double sum2 = 0.0;
        double sum3 = 0.0;

        for (; i + 4 <= n; i += 4) {
            sum0 += a[i]     * b[i];
            sum1 += a[i + 1] * b[i + 1];
            sum2 += a[i + 2] * b[i + 2];
            sum3 += a[i + 3] * b[i + 3];
        }

        double result = (sum0 + sum1) + (sum2 + sum3);

        // Handle the remaining 0 to 3 elements.
        for (; i < n; ++i) {
            result += a[i] * b[i];
        }

        return result;
    }
};

// Measure average execution time in milliseconds.
template <typename Function>
double benchmarkMs(Function&& function, int repetitions)
{
    function(); // Warm-up

    const auto start = std::chrono::steady_clock::now();

    for (int r = 0; r < repetitions; ++r) {
        function();
    }

    const auto end = std::chrono::steady_clock::now();

    return std::chrono::duration<double, std::milli>(
        end - start).count() / repetitions;
}

bool matricesClose(
    const MatrixOptimization::Matrix& A,
    const MatrixOptimization::Matrix& B,
    double tolerance = 1e-10)
{
    for (std::size_t i = 0; i < A.size(); ++i) {
        for (std::size_t j = 0; j < A[i].size(); ++j) {
            if (std::abs(A[i][j] - B[i][j]) >
                tolerance * (1.0 + std::abs(A[i][j]))) {
                return false;
            }
        }
    }

    return true;
}

int main(int argc, char* argv[])
{

    if(argc != 5){
       std::cerr << "Usage: " << argv[0] << " N " << " n " << " R " << " r\n";
       return 1;
    }

    const long long  N = std::stoll(argv[1]);
    const long long  VECTOR_SIZE = std::stoll(argv[2]);
    const long long  MATRIX_REPS = std::stoll(argv[3]);
    const long long  DOT_REPS = std::stoll(argv[4]);

    using Matrix = MatrixOptimization::Matrix;

    std::mt19937 gen(12345);
    std::uniform_real_distribution<double> distribution(-1.0, 1.0);

    Matrix A(N, std::vector<double>(N));
    Matrix B(N, std::vector<double>(N));
    Matrix C_basic(N, std::vector<double>(N, 0.0));
    Matrix C_optimized(N, std::vector<double>(N, 0.0));

    std::vector<double> a(VECTOR_SIZE);
    std::vector<double> b(VECTOR_SIZE);

    // Initialize matrices and vectors.
    for (std::size_t i = 0; i < N; ++i) {
        for (std::size_t j = 0; j < N; ++j) {
            A[i][j] = distribution(gen);
            B[i][j] = distribution(gen);
        }
    }

    for (std::size_t i = 0; i < VECTOR_SIZE; ++i) {
        a[i] = distribution(gen);
        b[i] = distribution(gen);
    }

    // Matrix multiplication benchmark.
    const double basicMatrixMs = benchmarkMs([&]() {
        MatrixOptimization::matrixMultiplyBasic(A, B, C_basic);
    }, MATRIX_REPS);

    const double optimizedMatrixMs = benchmarkMs([&]() {
        MatrixOptimization::matrixMultiplyOptimized(
            A, B, C_optimized);
    }, MATRIX_REPS);

    // Dot product benchmark.
    volatile double result_sink = 0.0;

    const double basicDotMs = benchmarkMs([&]() {
        result_sink = MatrixOptimization::dotProductBasic(a, b);
    }, DOT_REPS);

    const double unrolledDotMs = benchmarkMs([&]() {
        result_sink = MatrixOptimization::dotProductUnrolled(a, b);
    }, DOT_REPS);

    // Numerical correctness checks.
    const double dot_basic =
        MatrixOptimization::dotProductBasic(a, b);

    const double dot_unrolled =
        MatrixOptimization::dotProductUnrolled(a, b);

    const double dot_error = std::abs(dot_basic - dot_unrolled);

    const double dot_tolerance =
        1e-10 * (1.0 + std::abs(dot_basic));

    // Display results.
    std::cout << std::fixed << std::setprecision(6);

    std::cout << "Matrix size: " << N << " x " << N << '\n';
    std::cout << "Vector length: " << VECTOR_SIZE << "\n\n";

    std::cout << "=== Matrix multiplication ===\n";
    std::cout << "Basic (i-j-k):     "
              << basicMatrixMs << " ms\n";
    std::cout << "Optimized (i-k-j): "
              << optimizedMatrixMs << " ms\n";
    std::cout << "Speedup: "
              << basicMatrixMs / optimizedMatrixMs << "x\n";
    std::cout << "Numerical check: "
              << (matricesClose(C_basic, C_optimized)
                      ? "PASS" : "FAIL")
              << "\n\n";

    std::cout << "=== Dot product ===\n";
    std::cout << "Basic:       " << basicDotMs << " ms\n";
    std::cout << "Unrolled x4: " << unrolledDotMs << " ms\n";
    std::cout << "Speedup: "
              << basicDotMs / unrolledDotMs << "x\n";
    std::cout << "Basic result:    " << dot_basic << '\n';
    std::cout << "Unrolled result: " << dot_unrolled << '\n';
    std::cout << "Absolute error:  " << dot_error << '\n';
    std::cout << "Numerical check: "
              << (dot_error <= dot_tolerance ? "PASS" : "FAIL")
              << '\n';

    return 0;
}
