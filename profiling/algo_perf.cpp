#include <cstddef>
#include <cstdlib>
#include <iostream>
#include <utility>
#include <vector>
#include <algorithm>
#include <random>
#include <unordered_map>
#include <numeric>
#include <iomanip>

class PrecisionTimer {
private:
    // TODO: Add timing infrastructure
    using Clock = std::chrono::high_resolution_clock;
    Clock::time_point startTime ;
    
public:
    PrecisionTimer(): startTime (Clock::now()) {
        // Initialize timing system
    }
    
    void startTiming() {
        // Record start time using high_resolution_clock
        startTime = Clock::now();
    }
    
    double getElapsedMicroseconds() {
        // Calculate elapsed time in microseconds
        const Clock::time_point endTime = Clock::now();
        return std::chrono::duration_cast<std::chrono::microseconds>(endTime-startTime).count();
    }
    
    double getElapsedMilliseconds() {
        // Calculate elapsed time in milliseconds
        const Clock::time_point endTime = Clock::now();
        return std::chrono::duration_cast<std::chrono::milliseconds>(endTime-startTime).count();
    }
};

class StatisticalBenchmark {
private:
    std::vector<double> measurements;
    std::string benchmarkName;

    // Approximate 95% Student-t critical values for df = 1..30.
    // For larger degrees of freedom, use the normal approximation.
    static double tCritical95(std::size_t degreesOfFreedom) {
        static const double values[] = {
            0.0,
            12.706, 4.303, 3.182, 2.776, 2.571,
            2.447, 2.365, 2.306, 2.262, 2.228,
            2.201, 2.179, 2.160, 2.145, 2.131,
            2.120, 2.110, 2.101, 2.093, 2.086,
            2.080, 2.074, 2.069, 2.064, 2.060,
            2.056, 2.052, 2.048, 2.045, 2.042
        };

        if (degreesOfFreedom == 0)
            return std::numeric_limits<double>::quiet_NaN();

        if (degreesOfFreedom <= 30)
            return values[degreesOfFreedom];

        return 1.96;
    }
    
public:
    StatisticalBenchmark(const std::string& name) : benchmarkName(name) {}
    
    template<typename Func>
    void runBenchmark(Func function, int trials = 10) {
        // Implement comprehensive benchmarking
        // 1. Clear previous measurements
        // 2. Perform warm-up run
        // 3. Execute function 'trials' times
        // 4. Record timing for each execution
        // 5. Store measurements for statistical analysis
        if (trials < 2) {
            throw std::invalid_argument(
                "At least 2 trials are required for statistical analysis."
            );
        }
        measurements.clear();
        measurements.reserve(static_cast<std::size_t>(trials));
        function();
       
        double elapseTime = 0.0;
        for(size_t i = 0; i < trials; i++){

            PrecisionTimer timerObject;
            timerObject.startTiming();
            function();
            elapseTime = timerObject.getElapsedMicroseconds();
            measurements.push_back(elapseTime);
        }
    }


    template<typename Func, typename Setup>
    void runBenchmark(Func function, Setup setup, int trials = 10) {
        if (trials < 2) {
            throw std::invalid_argument(
                "At least 2 trials are required."
            );
        }

        measurements.clear();
        measurements.reserve(static_cast<std::size_t>(trials));

        // Warm-up: setup and execution are excluded from measurements.
        setup();
        function();

        for (int i = 0; i < trials; ++i) {
            setup(); // Prepare data before timing.

            PrecisionTimer timerObject;
            timerObject.startTiming();

            function();

            measurements.push_back(
                timerObject.getElapsedMicroseconds()
            );
        }
    }

    
    void printStatistics() {
        // TODO: Calculate and display statistical analysis
        // - Average execution time
        // - Minimum and maximum times
        // - Standard deviation
        // - 95% confidence interval
        // - Coefficient of variation
        if(measurements.empty()) return;
        double mean = std::accumulate(measurements.begin(), measurements.end(), 0.0) /  measurements.size();
        auto [min_it, max_it] = std::minmax_element(measurements.begin(), measurements.end());
        double min = (min_it != measurements.end()) ? *min_it : 0.0;
        double max = (max_it != measurements.end()) ? *max_it : 0.0;
        double variance = 0.0;
        for(auto& m:measurements){
            variance += (m-mean)*(m-mean);
        }
        double stddev = std::sqrt(variance / (measurements.size() - 1));

        double cv = (mean != 0.0) ? (stddev / std::abs(mean)) * 100.0 : 0.0;

        const double standardError =
            stddev / std::sqrt(static_cast<double>(measurements.size()));
        const double criticalValue = tCritical95(measurements.size() - 1);
        const double marginOfError = criticalValue * standardError;

        std::cout << "\n=== " << benchmarkName << " ===\n";
        std::cout << std::fixed << std::setprecision(4);

        std::cout << "Trials:                 " << measurements.size() << '\n';
        std::cout << "Mean:                   " << mean << " us\n";
        std::cout << "Minimum:                " << min << " us\n";
        std::cout << "Maximum:                " << max << " us\n";
        std::cout << "Sample std. deviation:  "  << stddev << " us\n";
        std::cout << "Standard error:         " << standardError << " us\n";

        std::cout << "95% CI for mean:        ["
                  << mean - marginOfError << ", "
                  << mean + marginOfError << "] us\n";

        std::cout << "Coefficient of variation: " << cv << "%\n";
    }
    
    double getAverageTime() const {
        // Return average execution time
        if (measurements.empty())
            return 0.0;

        return std::accumulate(
            measurements.begin(),
            measurements.end(),
            0.0
        ) / measurements.size();
    }
    
    double getStandardDeviation() const {
        // Calculate standard deviation of measurements
        const std::size_t n = measurements.size();

        if (n < 2)
            return 0.0;

        const double mean = getAverageTime();
        double sumSquaredDifferences = 0.0;

        for (double value : measurements) {
            const double difference = value - mean;
            sumSquaredDifferences += difference * difference;
        }

        // Sample standard deviation (Bessel's correction).
        return std::sqrt(
            sumSquaredDifferences / static_cast<double>(n - 1)
        );
    }
};


class AlgorithmProfiler {
private:
    std::vector<int> testData;
    
public:
    const std::vector<int>& data() const {
        return testData;
    }
    void generateTestData(size_t size) {
        // Generate realistic test dataset
        // Use random data with controlled distribution
        testData.clear();
        testData.reserve(static_cast<size_t>(size));
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<> dis(-1, size*10);
        for(size_t i = 0 ; i < size; i++) testData.push_back(dis(gen));
    }
    
    // Inefficient O(n²) sorting algorithm for comparison
    void bubbleSort(std::vector<int>& data) {
        // Implement bubble sort for profiling comparison
        // This intentionally inefficient algorithm helps demonstrate
        // the dramatic impact of algorithmic complexity
        size_t n = data.size();
        for(size_t i = 0; i < n-1; ++i){
            for(size_t j = 0; j < n-i-1; j++){
                if(data[j] > data[j+1]){
                    std::swap(data[j], data[j+1]);
                }
            }
        }
    }
    
    // Efficient O(n log n) sorting using STL
    void optimizedSort(std::vector<int>& data) {
        // Use std::sort for comparison
        std::sort(data.begin(), data.end());
    }
    
    // Linear search O(n)
    int linearSearch(const std::vector<int>& data, int target) {
        // Implement simple linear search
        for (std::size_t i = 0; i < data.size(); ++i) {
            if (data[i] == target)
                return static_cast<int>(i);
        }
        return -1;
    }
    
    // Binary search O(log n) - requires sorted data
    int binarySearch(const std::vector<int>& data, int target) {
        // Implement binary search algorithm
        std::size_t left (0), right(data.size());
         while(left < right){
            const std::size_t mid = left + (right - left) / 2;

            if (data[mid] == target)
                return static_cast<int>(mid);

            if(data[mid] < target) left = mid + 1;
            else right = mid ;
         }
        return -1;
    }
    
    // Hash-based lookup O(1) average case
    std::unordered_map<int, int> buildHashIndex(const std::vector<int>& data) {
       std::unordered_map<int, int> hashMap;
       hashMap.reserve(data.size());
        for (std::size_t i = 0; i < data.size(); ++i) {
            hashMap.try_emplace(
                data[i],
                static_cast<int>(i)
            );
        }

        // TBuild hash map for O(1) lookups
        return hashMap;
    }
    
    int hashLookup(const std::unordered_map<int, int>& hashMap, int target) {
        // Perform hash-based lookup
        auto it = hashMap.find(target);
        return (it != hashMap.end()) ? it->second : -1;
    }
    
    // Memory-intensive operation for cache analysis
    void processDataSequential(const std::vector<int>& data) {
        // Sequential memory access pattern (cache-friendly)
        volatile long sum = 0;
        for (size_t i = 0; i < data.size(); ++i) {
            sum += data[i];
        }
    }
    
    void processDataRandom(const std::vector<int>& data) {
        // Random memory access pattern (cache-unfriendly)
        std::vector<size_t> indices(data.size());
        std::iota(indices.begin(), indices.end(), 0);
       std::default_random_engine rng(std::random_device{}()); 
        std::shuffle(indices.begin(), indices.end(), rng);

        
        volatile long sum = 0;
        for (size_t idx : indices) {
            sum += data[idx];
        }
    }
};

volatile std::uint64_t benchmarkSink = 0;


int main() {
    AlgorithmProfiler profiler;

    const std::vector<std::size_t> sizes = {
        1'000, 10'000, 100'000
    };

    constexpr int sortTrials = 5;
    constexpr int otherTrials = 20;
    constexpr int searchQueries = 100;

    const int missingTarget = std::numeric_limits<int>::max();

    for (std::size_t n : sizes) {
        profiler.generateTestData(n);
        const std::vector<int> original = profiler.data();

        std::cout << "\n================================\n"
                  << "Dataset size: " << n << '\n'
                  << "================================\n";

        // 1. std::sort: exclude vector copying from timing.
        std::vector<int> sortWork;

        StatisticalBenchmark sortBenchmark(
            "std::sort"
        );

        sortBenchmark.runBenchmark(
            [&]() {
                profiler.optimizedSort(sortWork);
            },
            [&]() {
                sortWork = original;
            },
            sortTrials
        );

        sortBenchmark.printStatistics();

        // 2. Bubble sort: skip 100K because of O(n^2) cost.
        if (n <= 10'000) {
            std::vector<int> bubbleWork;

            StatisticalBenchmark bubbleBenchmark(
                "Bubble sort"
            );

            bubbleBenchmark.runBenchmark(
                [&]() {
                    profiler.bubbleSort(bubbleWork);
                },
                [&]() {
                    bubbleWork = original;
                },
                sortTrials
            );

            bubbleBenchmark.printStatistics();
        } else {
            std::cout
                << "\nBubble sort skipped for 100K elements.\n";
        }

        // Prepare search structures outside timed regions.
        std::vector<int> sorted = original;
        std::sort(sorted.begin(), sorted.end());

        const auto hashIndex =
            profiler.buildHashIndex(original);

        // 3. Linear search: absent target forces a full scan.
        StatisticalBenchmark linearBenchmark(
            "Linear search (100 queries)"
        );

        linearBenchmark.runBenchmark([&]() {
            volatile int result = -1;

            for (int q = 0; q < searchQueries; ++q) {
                result = profiler.linearSearch(
                    original, missingTarget
                );
            }
        }, otherTrials);

        linearBenchmark.printStatistics();

        // 4. Binary search: requires sorted input.
        StatisticalBenchmark binaryBenchmark(
            "Binary search (100 queries)"
        );

        binaryBenchmark.runBenchmark([&]() {
            volatile int result = -1;

            for (int q = 0; q < searchQueries; ++q) {
                result = profiler.binarySearch(
                    sorted, missingTarget
                );
            }
        }, otherTrials);

        binaryBenchmark.printStatistics();

        // 5. Hash lookup: hash index construction excluded.
        StatisticalBenchmark hashBenchmark(
            "Hash lookup (100 queries)"
        );

        hashBenchmark.runBenchmark([&]() {
            volatile int result = -1;

            for (int q = 0; q < searchQueries; ++q) {
                result = profiler.hashLookup(
                    hashIndex, missingTarget
                );
            }
        }, otherTrials);

        hashBenchmark.printStatistics();

        // 6. Sequential memory access.
        StatisticalBenchmark sequentialBenchmark(
            "Sequential memory access"
        );

        sequentialBenchmark.runBenchmark([&]() {
            profiler.processDataSequential(original);
        }, otherTrials);

        sequentialBenchmark.printStatistics();

        // 7. Randomized memory access.
        std::vector<std::size_t> indices(n);
        std::iota(indices.begin(), indices.end(), 0);

        std::mt19937 rng(12345);
        std::shuffle(indices.begin(), indices.end(), rng);

        // IMPORTANT:
        // Your original processDataRandom(data) creates and shuffles
        // indices internally. That includes setup in the timed region.
        // Add the overload shown below to measure access alone.
        StatisticalBenchmark randomBenchmark(
            "Randomized memory access"
        );

        randomBenchmark.runBenchmark([&]() {
            profiler.processDataRandom(original);
        }, otherTrials);

        randomBenchmark.printStatistics();
    }

    return 0;
}
