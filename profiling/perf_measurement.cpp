#include <cstddef>
#include <cstdlib>
#include <iostream>
#include <chrono>
#include <locale>
#include <memory>
#include <vector>
#include <algorithm>
#include <numeric>
#include <iomanip>
#include <cmath>
#include <functional>

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


int main() {
    // Test 1: arithmetic workload.
    volatile long long arithmeticResult = 0;

    StatisticalBenchmark arithmetic("Arithmetic workload");

    arithmetic.runBenchmark([&]() {
        long long sum = 0;

        for (int i = 0; i < 10000; ++i) {
            sum += static_cast<long long>(i) * i;
        }

        arithmeticResult = sum;
    }, 100);

    arithmetic.printStatistics();

    // Test 2: vector initialization and accumulation.
    std::vector<int> data(10000);

    StatisticalBenchmark vectorTest("Vector workload");

    vectorTest.runBenchmark([&]() {
        std::iota(data.begin(), data.end(), 1);

        const long long sum = std::accumulate(
            data.begin(), data.end(), 0LL
        );

        // Make the result observable.
        arithmeticResult = sum;
    }, 100);

    vectorTest.printStatistics();

    return 0;
}