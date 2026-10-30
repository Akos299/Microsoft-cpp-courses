// ============================================================================
// Basic Code Optimization Lab - Starter Files
// Overview: Learn memory optimization, algorithm analysis, and compiler optimization
// Duration: 90 minutes
// Instructions: Follow the TODOs to implement fundamental optimization techniques
// ============================================================================

#include <cstddef>
#include <iostream>
#include <vector>
#include <chrono>
#include <random>
#include <algorithm>
#include <numeric>
#include <iomanip>
#include <cmath>
#include <unordered_map>

using namespace std;

// Constants for testing
const int SMALL_SIZE = 1000;
const int LARGE_SIZE = 10000;
const int NUM_ITERATIONS = 5;


// Simple timer class for performance measurement
class SimpleTimer {
private:
    chrono::high_resolution_clock::time_point start_time;
    
public:
    void start() {
        start_time = chrono::high_resolution_clock::now();
    }

    double getElapsedMs() {
        const auto end_time = chrono::steady_clock::now();
        return chrono::duration<double, std::milli>(
            end_time - start_time).count();
    }

};

// ============================================================================
// PART 1: Memory Optimization Techniques
// ============================================================================

// Unoptimized data structure (Array of Structures)
struct UnoptimizedParticle {
    double x, y, z;      // Position
    double vx, vy, vz;   // Velocity  
    double mass;
    int id;
    char padding[4];     // Unintentional padding
};

// Create optimized data structure using Structure of Arrays (SoA)
class OptimizedParticleSystem {
private:
    // Create separate arrays for each property to improve cache locality:
    // - vector<double> for x, y, z positions
    // - vector<double> for vx, vy, vz velocities  
    // - vector<double> for mass
    // - vector<int> for id

    std::vector<double> pos_x;
    std::vector<double> pos_y;
    std::vector<double> pos_z;
    std::vector<double>  vel_x;
    std::vector<double>  vel_y;
    std::vector<double>  vel_z;
    std::vector<double>  mass_;
    std::vector<int> id_;

    
public:
    size_t size() const {
      
        return id_.size(); 
    }
    
    void resize(size_t n) {
        // Resize all arrays to size n
        if (n == 0)
                throw std::invalid_argument("n must be positive");
            pos_x.resize(n);
            pos_y.resize(n);
            pos_z.resize(n);

            vel_x.resize(n);
            vel_y.resize(n);
            vel_z.resize(n);

            mass_.resize(n);
            id_.resize(n);
    }
    
    void setParticle(size_t i, double x, double y, double z, 
                    double vx, double vy, double vz, double mass, int id) { 
        // Set values in the separate arrays at index i
        pos_x[i] = x; pos_y[i] = y; pos_z[i] = z;
        vel_x[i] = vx; vel_y[i] = vy; vel_z[i] = vz;
        mass_[i] = mass; id_[i] = id;
        
    }
    
    void updatePositions(double dt) {
        // Update positions: x[i] += vx[i] * dt (and similar for y, z)
        // This should be cache-friendly due to sequential access
        for(size_t i = 0; i < size(); i++){
            pos_x[i] += vel_x[i] * dt;
            pos_y[i] += vel_y[i] * dt;
            pos_z[i] += vel_z[i] * dt;
        }
    }
    
    double getTotalKineticEnergy() const {
        // Calculate sum of 0.5 * mass[i] * (vx[i]² + vy[i]² + vz[i]²) for all particles
        double sum =0.;
        for(size_t i = 0 ; i < size(); i++)
        {
            double vel2 = vel_x[i] * vel_x[i] + vel_y[i] * vel_y[i] + vel_z[i] * vel_z[i];
            sum += 0.5 * mass_[i] * vel2;
        }
        return sum; 
    }
};

// Implement simple memory pool for frequent allocations

template<typename T>
class SimpleMemoryPool {
private:
    std::vector<T> poolStorage;
    std::vector<bool> availableSlot;
    std::size_t poolSize;

public:
    explicit SimpleMemoryPool(std::size_t size)
        : poolStorage(size),
          availableSlot(size, true),
          poolSize(size)
    {
        if (size == 0) {
            throw std::invalid_argument("Pool size must be positive");
        }
    }

    T* allocate() {
        for (std::size_t i = 0; i < poolSize; ++i) {
            if (availableSlot[i]) {
                availableSlot[i] = false;
                return &poolStorage[i];
            }
        }

        return nullptr; // Pool full
    }

    void deallocate(T* ptr) {
        if (ptr == nullptr) {
            return;
        }

        // Search by pointer equality, without unsafe pointer arithmetic.
        for (std::size_t i = 0; i < poolSize; ++i) {
            if (&poolStorage[i] == ptr) {
                if (!availableSlot[i]) {
                    availableSlot[i] = true;
                    return;
                }

                throw std::invalid_argument(
                    "Slot already deallocated");
            }
        }

        throw std::invalid_argument(
            "Pointer does not belong to this pool");
    }

    std::size_t getPoolSize() const {
        return poolSize;
    }
};

// ============================================================================
// PART 2: Algorithm Analysis and Optimization
// ============================================================================

// Implement and compare different search algorithms
class SearchAlgorithms {
public:
    // Linear search - O(n)
    static int linearSearch(const vector<int>& data, int target) {
       for (std::size_t i = 0; i < data.size(); ++i) {
            if (data[i] == target)
                return static_cast<int>(i);
        }

        return -1;
    }
    
    // Binary search - O(log n) - requires sorted data
    static int binarySearch(const vector<int>& data, int target) {
         std::size_t left (0), right(data.size());
         while(left < right){
            const std::size_t mid = left + (right - left) / 2;

            if (data[mid] == target)
                return static_cast<int>(mid);

            if(data[mid] < target) left = mid + 1;
            else right = mid ;
         }
        return -1;  // Replace with actual implementation
    }
    
    // Hash-based search using unordered_map - O(1) average
    static void buildHashMap(const vector<int>& data, unordered_map<int, int>& hashMap) {
        hashMap.clear();
        hashMap.reserve(data.size());

        for (std::size_t i = 0; i < data.size(); ++i) {
            hashMap.try_emplace(
                data[i],
                static_cast<int>(i)
            );
        }
    }
    
    static int hashSearch(const unordered_map<int, int>& hashMap, int target) {
    
        auto it = hashMap.find(target);
        return (it != hashMap.end()) ? it->second : -1;
    }
};

// Optimize sorting algorithm selection
class SortingOptimization {
public:
    // Bubble sort - O(n²) - inefficient but simple
    static void bubbleSort(vector<int>& data) {
        size_t n = data.size();
        for (size_t i = 0; i < n - 1; ++i) {
            for (size_t j = 0; j < n - i - 1; ++j) {
                if (data[j] > data[j + 1]) {
                    swap(data[j], data[j + 1]);
                }
            }
        }
    }
    
    // Use STL sort - O(n log n) - highly optimized
    static void stlSort(vector<int>& data) {
        std::sort(data.begin(), data.end());
    }
    
    // Optimized for small arrays - hybrid approach
    static void hybridSort(vector<int>& data) {
        // If array size <= 20, use insertion sort (better for small arrays)
        // Otherwise, use std::sort
        
        if (data.size() <= 20) {
            insertionSort(data);
        } else {
            stlSort(data);
        }
    }
    
private:
    static void insertionSort(vector<int>& data) {
        // Simple insertion sort for small arrays
        for (size_t i = 1; i < data.size(); ++i) {
            int key = data[i];
            int j = i - 1;
            while (j >= 0 && data[j] > key) {
                data[j + 1] = data[j];
                j--;
            }
            data[j + 1] = key;
        }
    }
};

// ============================================================================
// PART 3: Loop and Computational Optimizations
// ============================================================================

// Implement loop optimizations
class LoopOptimizations {
public:
    // Unoptimized matrix multiplication
    static void matrixMultiplyBasic(const vector<vector<double>>& A,
                                   const vector<vector<double>>& B,
                                   vector<vector<double>>& C) {
        int n = A.size();
        for (std::size_t i = 0; i < n; ++i) {
            for (std::size_t j = 0; j < n; ++j) {
                C[i][j] = 0.0;

                for (std::size_t k = 0; k < n; ++k) {
                    C[i][j] += A[i][k] * B[k][j];
                }
            }
        }
    }
    
    // Cache-optimized matrix multiplication with loop interchange
    static void matrixMultiplyOptimized(const vector<vector<double>>& A,
                                      const vector<vector<double>>& B,  
                                      vector<vector<double>>& C) {
        int n = A.size();
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
    
    // Implement loop unrolling for vector operations
    static double dotProductBasic(const vector<double>& a, const vector<double>& b) {
        double result = 0.0;
        if (a.size() != b.size()) {
            throw std::invalid_argument("Vectors must have same size");
        }

        for (std::size_t i = 0; i < a.size(); ++i) {
            result += a[i] * b[i];
        }

        return result;
    }
    
    static double dotProductUnrolled(const vector<double>& a, const vector<double>& b) {
        if (a.size() != b.size()) {
            throw std::invalid_argument("Vectors must have same size");
        }

        const std::size_t n = a.size();
        std::size_t i = 0;

        double sum0 = 0.0;
        double sum1 = 0.0;
        double sum2 = 0.0;
        double sum3 = 0.0;

        for (; i + 3 < n; i += 4) {
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

// ============================================================================
// PART 4: Performance Testing Framework
// ============================================================================

class PerformanceTester {
private:
    SimpleTimer timer;
    
public:
    // TODO 7: Implement performance comparison framework
    template<typename Func>
    double measurePerformance(Func function, const string& description) {
        cout << "Testing: " << description << "..." << endl;
        
        // YOUR CODE HERE
        // 1. Start timer
        // 2. Run function NUM_ITERATIONS times
        // 3. Calculate average time
        // 4. Print results
        
        timer.start();
        for (int i = 0; i < NUM_ITERATIONS; ++i) {
            function();
        }
        double totalTime = timer.getElapsedMs();
        double avgTime = totalTime / NUM_ITERATIONS;
        
        cout << "  Average time: " << fixed << setprecision(3) << avgTime << " ms" << endl;
        return avgTime;
    }
    
    void comparePerformance(double baselineTime, double optimizedTime, const string& optimization) {
        // YOUR CODE HERE
        // Calculate speedup and improvement percentage
        // Print comparison results
        
        double speedup = baselineTime / optimizedTime;
        double improvement = ((baselineTime - optimizedTime) / baselineTime) * 100.0;
        
        cout << "  " << optimization << ":" << endl;
        cout << "    Speedup: " << fixed << setprecision(2) << speedup << "x" << endl;
        cout << "    Improvement: " << fixed << setprecision(1) << improvement << "%" << endl;
    }
};

// Test data generators
vector<int> generateRandomData(int size) {
    vector<int> data(size);
    random_device rd;
    mt19937 gen(rd());
    uniform_int_distribution<> dis(1, size * 10);
    
    for (int& val : data) {
        val = dis(gen);
    }
    return data;
}

vector<UnoptimizedParticle> generateParticles(int count) {
    vector<UnoptimizedParticle> particles(count);
    random_device rd;
    mt19937 gen(rd());
    uniform_real_distribution<double> pos(-100.0, 100.0);
    uniform_real_distribution<double> vel(-10.0, 10.0);
    uniform_real_distribution<double> mass(0.1, 10.0);
    
    for (int i = 0; i < count; ++i) {
        particles[i] = {pos(gen), pos(gen), pos(gen),
                       vel(gen), vel(gen), vel(gen),
                       mass(gen), i, {0}};
    }
    return particles;
}

// Main testing functions
void testMemoryOptimizations() {
    cout << "\n=== Memory Optimization Tests ===" << endl;
    
    PerformanceTester tester;
    auto particles = generateParticles(LARGE_SIZE);
    
    // Test Array of Structures vs Structure of Arrays
    auto aosTest = [&particles]() {
        double totalEnergy = 0.0;
        for (const auto& p : particles) {
            double vel2 = p.vx * p.vx + p.vy * p.vy + p.vz * p.vz;
            totalEnergy += 0.5 * p.mass * vel2;
        }
    };
    
    OptimizedParticleSystem soaParticles;
    soaParticles.resize(LARGE_SIZE);
    for (size_t i = 0; i < particles.size(); ++i) {
        soaParticles.setParticle(i, particles[i].x, particles[i].y, particles[i].z,
                                particles[i].vx, particles[i].vy, particles[i].vz,
                                particles[i].mass, particles[i].id);
    }
    
    auto soaTest = [&soaParticles]() {
        double totalEnergy = soaParticles.getTotalKineticEnergy();
    };
    
    double aosTime = tester.measurePerformance(aosTest, "Array of Structures");
    double soaTime = tester.measurePerformance(soaTest, "Structure of Arrays");
    
    tester.comparePerformance(aosTime, soaTime, "SoA vs AoS");
    
    // Test memory pool
    cout << "\nTesting memory pool allocation..." << endl;
    SimpleMemoryPool<int> pool(1000);
    
    auto poolTest = [&pool]() {
        vector<int*> ptrs;
        for (int i = 0; i < 100; ++i) {
            ptrs.push_back(pool.allocate());
        }
        for (int* ptr : ptrs) {
            if (ptr) pool.deallocate(ptr);
        }
    };
    
    auto heapTest = []() {
        vector<int*> ptrs;
        for (int i = 0; i < 100; ++i) {
            ptrs.push_back(new int(i));
        }
        for (int* ptr : ptrs) {
            delete ptr;
        }
    };
    
    double poolTime = tester.measurePerformance(poolTest, "Memory Pool");
    double heapTime = tester.measurePerformance(heapTest, "Heap Allocation");
    
    tester.comparePerformance(heapTime, poolTime, "Memory Pool vs Heap");
}

void testAlgorithmOptimizations() {
    cout << "\n=== Algorithm Optimization Tests ===" << endl;
    
    PerformanceTester tester;
    auto data = generateRandomData(LARGE_SIZE);
    auto sortedData = data;
    sort(sortedData.begin(), sortedData.end());
    
    int target = data[data.size() / 2];  // Pick middle element
    
    // Search algorithm comparison
    auto linearTest = [&data, target]() {
        SearchAlgorithms::linearSearch(data, target);
    };
    
    auto binaryTest = [&sortedData, target]() {
        SearchAlgorithms::binarySearch(sortedData, target);
    };
    
    unordered_map<int, int> hashMap;
    SearchAlgorithms::buildHashMap(data, hashMap);
    auto hashTest = [&hashMap, target]() {
        SearchAlgorithms::hashSearch(hashMap, target);
    };
    
    double linearTime = tester.measurePerformance(linearTest, "Linear Search");
    double binaryTime = tester.measurePerformance(binaryTest, "Binary Search");
    double hashTime = tester.measurePerformance(hashTest, "Hash Search");
    
    tester.comparePerformance(linearTime, binaryTime, "Binary vs Linear");
    tester.comparePerformance(linearTime, hashTime, "Hash vs Linear");
    
    // Sorting algorithm comparison
    auto testData = generateRandomData(SMALL_SIZE);
    
    auto bubbleTest = [&testData]() {
        auto data = testData;
        SortingOptimization::bubbleSort(data);
    };
    
    auto stlTest = [&testData]() {
        auto data = testData;
        SortingOptimization::stlSort(data);
    };
    
    double bubbleTime = tester.measurePerformance(bubbleTest, "Bubble Sort");
    double stlTime = tester.measurePerformance(stlTest, "STL Sort");
    
    tester.comparePerformance(bubbleTime, stlTime, "STL vs Bubble");
}

void testLoopOptimizations() {
    cout << "\n=== Loop Optimization Tests ===" << endl;
    
    PerformanceTester tester;
    const int matrixSize = 200;  // Small enough for reasonable test time
    
    // Initialize matrices
    vector<vector<double>> A(matrixSize, vector<double>(matrixSize, 1.0));
    vector<vector<double>> B(matrixSize, vector<double>(matrixSize, 2.0));
    vector<vector<double>> C1(matrixSize, vector<double>(matrixSize, 0.0));
    vector<vector<double>> C2(matrixSize, vector<double>(matrixSize, 0.0));
    
    auto basicMatrixTest = [&]() {
        LoopOptimizations::matrixMultiplyBasic(A, B, C1);
    };
    
    auto optimizedMatrixTest = [&]() {
        LoopOptimizations::matrixMultiplyOptimized(A, B, C2);
    };
    
    double basicTime = tester.measurePerformance(basicMatrixTest, "Basic Matrix Multiply");
    double optimizedTime = tester.measurePerformance(optimizedMatrixTest, "Optimized Matrix Multiply");
    
    tester.comparePerformance(basicTime, optimizedTime, "Loop Interchange");
    
    // Vector operations
    vector<double> vec1(LARGE_SIZE, 1.5);
    vector<double> vec2(LARGE_SIZE, 2.5);
    
    auto basicDotTest = [&]() {
        LoopOptimizations::dotProductBasic(vec1, vec2);
    };
    
    auto unrolledDotTest = [&]() {
        LoopOptimizations::dotProductUnrolled(vec1, vec2);
    };
    
    double basicDotTime = tester.measurePerformance(basicDotTest, "Basic Dot Product");
    double unrolledDotTime = tester.measurePerformance(unrolledDotTest, "Unrolled Dot Product");
    
    tester.comparePerformance(basicDotTime, unrolledDotTime, "Loop Unrolling");
}

int main() {
    cout << "=== Basic Code Optimization Lab ===" << endl;
    cout << "Learning memory optimization, algorithm analysis, and compiler optimization" << endl;
    
    testMemoryOptimizations();
    testAlgorithmOptimizations();
    testLoopOptimizations();
    
    cout << "\n=== Compiler Optimization Reminder ===" << endl;
    cout << "Remember to test with different compiler flags:" << endl;
    cout << "  -O0 (no optimization)" << endl;
    cout << "  -O2 (standard optimization)" << endl;
    cout << "  -O3 (maximum optimization)" << endl;
    cout << "  -march=native (optimize for your CPU)" << endl;
    
    cout << "\n=== Lab Complete! ===" << endl;
    cout << "You've successfully implemented basic optimization techniques!" << endl;
    
    return 0;
}

/*
REQUIREMENTS CHECKLIST:
□ Implement Structure of Arrays (SoA) for better cache locality
□ Create simple memory pool for frequent allocations  
□ Implement and compare linear vs binary vs hash search algorithms
□ Replace inefficient sorting with optimized STL algorithms
□ Apply loop interchange for better cache performance in matrix operations
□ Use loop unrolling to reduce overhead in vector operations
□ Create performance measurement framework to validate optimizations
□ Demonstrate measurable performance improvements

COMMON MISTAKES TO AVOID:
- Not properly implementing SoA layout (mixing up array indices)
- Forgetting to handle edge cases in binary search (bounds checking)
- Not ensuring data is sorted before binary search
- Incorrect loop unrolling (not handling remaining elements)
- Memory pool not properly tracking free/used slots

VALIDATION:
- SoA should show performance improvement over AoS for large datasets
- Binary search should be much faster than linear for large sorted arrays
- STL sort should significantly outperform bubble sort
- Loop interchange should improve matrix multiplication performance
- Unrolled loops should show modest performance gains
*/
