#include <iostream>
#include <vector>
#include <algorithm>
#include <numeric>
#include <functional>
#include <iterator>
#include <type_traits>
#include <string>
#include <chrono>
#include <cmath>
#include <random>
#include <iomanip>


// Custom predicate functors for algorithm composition
template<typename T>
class ThresholdFilter {
private:
    T threshold;
    bool aboveThreshold;
    
public:
    ThresholdFilter(T thresh, bool above = true) 
        : threshold(thresh), aboveThreshold(above) {}
    
    bool operator()(const T& value) const {
        return aboveThreshold ? value > threshold : value < threshold;
    }
};

template<typename Container>
class StatisticalAnalyzer {
private:
    Container data;
    
public:
    StatisticalAnalyzer(const Container& container) : data(container) {}
    
    auto mean() const {
        using ValueType = typename Container::value_type;
        if (data.empty()) return ValueType{};
        
        auto sum = std::accumulate(data.begin(), data.end(), ValueType{});
        return sum / data.size();
    }
    
    auto median() const {
        using ValueType = typename Container::value_type;
        if (data.empty()) return ValueType{};
        
        Container sortedData = data;
        std::sort(sortedData.begin(), sortedData.end());
        
        size_t mid = sortedData.size() / 2;
        if (sortedData.size() % 2 == 0) {
            return (sortedData[mid - 1] + sortedData[mid]) / 2;
        } else {
            return sortedData[mid];
        }
    }
    
    auto standardDeviation() const {
        using ValueType = typename Container::value_type;
        if (data.size() <= 1) return ValueType{};
        
        auto meanValue = mean();
        auto variance = std::accumulate(data.begin(), data.end(), ValueType{},
            [meanValue](ValueType acc, const ValueType& val) {
                auto diff = val - meanValue;
                return acc + diff * diff;
            }) / (data.size() - 1);
        
        return std::sqrt(variance);
    }
};

// Custom algorithm: parallel-style processing simulation
template<typename InputIt, typename OutputIt, typename UnaryOperation>
OutputIt transform_parallel_sim(InputIt first, InputIt last, OutputIt d_first, UnaryOperation op) {
    // Simulate parallel processing by chunking data
    auto distance = std::distance(first, last);
    const size_t chunk_size = std::max(1L, distance / 4); // Simulate 4 threads
    
    auto current = first;
    auto output = d_first;
    
    while (current != last) {
        auto chunk_end = current;
        std::advance(chunk_end, std::min(chunk_size, static_cast<size_t>(std::distance(current, last))));
        
        // Process chunk (simulating parallel execution)
        output = std::transform(current, chunk_end, output, op);
        current = chunk_end;
    }
    
    return output;
}

// Custom algorithm: find_all (returns all matching elements)
template<typename Container, typename Predicate>
auto find_all(const Container& container, Predicate pred) {
    std::vector<typename Container::value_type> results;
    std::copy_if(container.begin(), container.end(), 
                std::back_inserter(results), pred);
    return results;
}

class CustomAlgorithmIntegration {
private:
    std::vector<double> performanceData;
    std::vector<int> categoricalData;
    
public:
    void generateOptimizationTestData() {
        performanceData.clear();
        categoricalData.clear();
        
        // Generate realistic performance metrics
        std::random_device rd;
        std::mt19937 gen(rd());
        std::normal_distribution<double> normalDist(100.0, 25.0);
        std::uniform_int_distribution<int> categoryDist(1, 10);
        
        for (int i = 0; i < 10000; ++i) {
            performanceData.push_back(std::max(0.0, normalDist(gen)));
            categoricalData.push_back(categoryDist(gen));
        }
        
        std::cout << "Generated " << performanceData.size() << " performance data points" << std::endl;
    }
    
    void demonstrateCustomPredicates() {
        std::cout << "\n=== CUSTOM PREDICATE ALGORITHMS ===" << std::endl;
        
        // Use custom threshold filters
        ThresholdFilter<double> highPerformance(120.0, true);
        ThresholdFilter<double> lowPerformance(80.0, false);
        
        auto highPerformers = find_all(performanceData, highPerformance);
        auto lowPerformers = find_all(performanceData, lowPerformance);
        
        std::cout << "High performers (>120): " << highPerformers.size() << std::endl;
        std::cout << "Low performers (<80): " << lowPerformers.size() << std::endl;
        
        // Demonstrate predicate composition
        auto extremePerformers = find_all(performanceData, [](double val) {
            return ThresholdFilter<double>(130.0, true)(val) || ThresholdFilter<double>(70.0, false)(val);
        });
        
        std::cout << "Extreme performers (>130 or <70): " << extremePerformers.size() << std::endl;
    }
    
    void demonstrateStatisticalAnalysis() {
        std::cout << "\n=== STATISTICAL ANALYSIS ALGORITHMS ===" << std::endl;
        
        StatisticalAnalyzer<std::vector<double>> analyzer(performanceData);
        
        auto mean = analyzer.mean();
        auto median = analyzer.median();
        auto stdDev = analyzer.standardDeviation();
        
        std::cout << std::fixed << std::setprecision(2);
        std::cout << "Performance Statistics:" << std::endl;
        std::cout << "  Mean: " << mean << std::endl;
        std::cout << "  Median: " << median << std::endl;
        std::cout << "  Standard Deviation: " << stdDev << std::endl;
        
        // Apply statistical analysis to filtered data
        ThresholdFilter<double> aboveAverage(mean, true);
        auto aboveAverageData = find_all(performanceData, aboveAverage);
        
        StatisticalAnalyzer<std::vector<double>> filteredAnalyzer(aboveAverageData);
        std::cout << "Above-average subset statistics:" << std::endl;
        std::cout << "  Count: " << aboveAverageData.size() << std::endl;
        std::cout << "  Mean: " << filteredAnalyzer.mean() << std::endl;
        std::cout << "  Std Dev: " << filteredAnalyzer.standardDeviation() << std::endl;
    }
    
    void demonstratePerformanceOptimization() {
        std::cout << "\n=== PERFORMANCE OPTIMIZATION COMPARISON ===" << std::endl;
        
        auto measureTime = [](auto&& func) {
            auto start = std::chrono::high_resolution_clock::now();
            func();
            auto end = std::chrono::high_resolution_clock::now();
            return std::chrono::duration_cast<std::chrono::microseconds>(end - start).count() / 1000.0;
        };
        
        // Standard transform performance
        std::vector<double> standardResults(performanceData.size());
        double standardTime = measureTime([&]() {
            std::transform(performanceData.begin(), performanceData.end(),
                          standardResults.begin(),
                          [](double val) { return val * 1.1; }); // 10% increase
        });
        
        // Custom "parallel" transform performance
        std::vector<double> parallelResults(performanceData.size());
        double parallelTime = measureTime([&]() {
            transform_parallel_sim(performanceData.begin(), performanceData.end(),
                                  parallelResults.begin(),
                                  [](double val) { return val * 1.1; });
        });
        
        std::cout << "Transform performance comparison:" << std::endl;
        std::cout << "  Standard transform: " << standardTime << " ms" << std::endl;
        std::cout << "  Parallel simulation: " << parallelTime << " ms" << std::endl;
        std::cout << "  Speedup: " << (standardTime / parallelTime) << "x" << std::endl;
        
        // Verify results are identical
        bool resultsMatch = std::equal(standardResults.begin(), standardResults.end(), 
                                     parallelResults.begin());
        std::cout << "  Results match: " << (resultsMatch ? "Yes" : "No") << std::endl;
    }
    
    void demonstrateAlgorithmComposition() {
        std::cout << "\n=== ADVANCED ALGORITHM COMPOSITION ===" << std::endl;
        
        // Complex multi-stage pipeline using custom algorithms
        auto pipeline_start = std::chrono::high_resolution_clock::now();
        
        // Step 1: Filter extreme values using custom predicate
        auto normalRange = find_all(performanceData, [](double val) {
            return val >= 50.0 && val <= 150.0;
        });
        
        // Step 2: Apply custom transformation
        std::vector<double> transformedData(normalRange.size());
        transform_parallel_sim(normalRange.begin(), normalRange.end(),
                              transformedData.begin(),
                              [](double val) { return std::log(val + 1); });
        
        // Step 3: Statistical analysis of transformed data
        StatisticalAnalyzer<std::vector<double>> transformedAnalyzer(transformedData);
        
        // Step 4: Final filtering based on statistical properties
        auto meanTransformed = transformedAnalyzer.mean();
        auto stdDevTransformed = transformedAnalyzer.standardDeviation();
        
        auto finalResults = find_all(transformedData, [meanTransformed, stdDevTransformed](double val) {
            return std::abs(val - meanTransformed) <= stdDevTransformed;
        });
        
        auto pipeline_end = std::chrono::high_resolution_clock::now();
        auto pipeline_time = std::chrono::duration_cast<std::chrono::microseconds>(
            pipeline_end - pipeline_start).count() / 1000.0;
        
        std::cout << "Complex pipeline results:" << std::endl;
        std::cout << "  Original data points: " << performanceData.size() << std::endl;
        std::cout << "  After range filtering: " << normalRange.size() << std::endl;
        std::cout << "  After statistical filtering: " << finalResults.size() << std::endl;
        std::cout << "  Pipeline execution time: " << pipeline_time << " ms" << std::endl;
        std::cout << "  Data reduction: " << std::fixed << std::setprecision(1)
                  << (100.0 * (performanceData.size() - finalResults.size()) / performanceData.size())
                  << "%" << std::endl;
    }
};

int main(){
    std::cout << "CUSTOM ALGORITHM INTEGRATION AND OPTIMIZATION" << std::endl;
    std::cout << "=============================================" << std::endl;
    
    CustomAlgorithmIntegration integration;
    
    // Generate test data
    integration.generateOptimizationTestData();
    
    // Demonstrate different aspects of custom algorithm integration
    integration.demonstrateCustomPredicates();
    integration.demonstrateStatisticalAnalysis();
    integration.demonstratePerformanceOptimization();
    integration.demonstrateAlgorithmComposition();
    
    std::cout << "\n=== KEY INSIGHTS ===" << std::endl;
    std::cout << "✓ Custom predicates improve code reusability and readability" << std::endl;
    std::cout << "✓ Statistical algorithms can be composed with STL algorithms" << std::endl;
    std::cout << "✓ Performance optimization often involves algorithm selection" << std::endl;
    std::cout << "✓ Complex pipelines benefit from modular, testable components" << std::endl;
    
    return 0;
}