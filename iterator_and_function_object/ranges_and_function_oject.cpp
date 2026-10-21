#include <iostream>
#include <vector>
#include <algorithm>
#include <numeric>
#include <functional>
#include <iomanip>
#include <chrono>


#if __cplusplus >= 202002L
#include <ranges>
namespace rng = std::ranges;
namespace views = std::views;
#define HAS_RANGES 1
#else
#define HAS_RANGES 0
#endif

// Function objects compatible with both traditional STL and ranges
class ScaleValue {
private:
    double factor;
    
public:
    ScaleValue(double f) : factor(f) {}
    
    template<typename T>
    auto operator()(T value) const -> decltype(value * factor) {
        return static_cast<decltype(value * factor)>(value * factor);
    }
};

class IsMultipleOf {
private:
    int divisor;
    
public:
    IsMultipleOf(int d) : divisor(d) {}
    
    bool operator()(int value) const {
        return divisor != 0 && value % divisor == 0;
    }
};

class RangeIntegration {
private:
    std::vector<int> dataSource;
    
public:
    void generateTestData() {
        dataSource.clear();
        
        // Create interesting data pattern
        for (int i = 1; i <= 100; ++i) {
            dataSource.push_back(i);
            if (i % 10 == 0) {
                dataSource.push_back(i); // Duplicates every 10th number
            }
        }
        
        std::cout << "Generated " << dataSource.size() << " test elements" << std::endl;
    }
    
    void demonstrateTraditionalApproach() {
        std::cout << "\n=== TRADITIONAL STL APPROACH ===" << std::endl;
        
        // Multi-step processing using traditional STL
        std::cout << "Step-by-step traditional processing:" << std::endl;
        
        // Step 1: Filter multiples of 5
        IsMultipleOf multiplesOf5(5);
        std::vector<int> filtered;
        std::copy_if(dataSource.begin(), dataSource.end(), 
                    std::back_inserter(filtered), multiplesOf5);
        
        std::cout << "  After filtering multiples of 5: " << filtered.size() << " elements" << std::endl;
        
        // Step 2: Scale by 1.5
        ScaleValue scaler(1.5);
        std::vector<double> scaled(filtered.size());
        std::transform(filtered.begin(), filtered.end(), scaled.begin(), scaler);
        
        std::cout << "  After scaling by 1.5: " << scaled.size() << " elements" << std::endl;
        
        // Step 3: Take first 10 elements
        std::vector<double> limited;
        auto takeCount = std::min(10ul, scaled.size());
        std::copy_n(scaled.begin(), takeCount, std::back_inserter(limited));
        
        std::cout << "  After limiting to 10: " << limited.size() << " elements" << std::endl;
        
        // Display results
        std::cout << "  Final results: ";
        for (double val : limited) {
            std::cout << std::fixed << std::setprecision(1) << val << " ";
        }
        std::cout << std::endl;
        
        std::cout << "Traditional approach: Multiple intermediate containers, explicit steps" << std::endl;
    }
    
    #if HAS_RANGES
    void demonstrateRangesApproach() {
        std::cout << "\n=== MODERN C++20 RANGES APPROACH ===" << std::endl;
        
        // Same processing using ranges - single expression
        IsMultipleOf multiplesOf5(5);
        ScaleValue scaler(1.5);
        
        auto pipeline = dataSource 
            | views::filter(multiplesOf5)
            | views::transform(scaler)
            | views::take(10);
        
        std::cout << "Single pipeline expression with function objects:" << std::endl;
        std::cout << "  Results: ";
        for (auto val : pipeline) {
            std::cout << std::fixed << std::setprecision(1) << val << " ";
        }
        std::cout << std::endl;
        
        // Demonstrate lazy evaluation
        std::cout << "\nLazy evaluation demonstration:" << std::endl;
        
        auto lazyPipeline = dataSource
            | views::transform([](int x) { 
                std::cout << "Processing: " << x << " -> ";
                return x * 2;
              })
            | views::filter([](int x) {
                std::cout << x << " (filtered: " << (x > 50 ? "pass" : "fail") << ") ";
                return x > 50;
              })
            | views::take(5);
        
        std::cout << "\n  Consuming lazy pipeline:" << std::endl;
        std::cout << "  Final results: ";
        for (auto val : lazyPipeline) {
            std::cout << val << " ";
        }
        std::cout << std::endl;
    }
    
    void demonstrateRangesWithFunctionObjects() {
        std::cout << "\n=== RANGES WITH CUSTOM FUNCTION OBJECTS ===" << std::endl;
        
        // Create various function objects
        IsMultipleOf multiplesOf3(3);
        IsMultipleOf multiplesOf7(7);
        ScaleValue doubler(2.0);
        ScaleValue halver(0.5);
        
        // Complex pipeline composition
        auto complexPipeline = dataSource
            | views::filter([&](int x) { return multiplesOf3(x) || multiplesOf7(x); })
            | views::transform(doubler)
            | views::filter([](double x) { return x < 100; })
            | views::transform(halver)
            | views::take(15);
        
        std::cout << "Complex pipeline (multiples of 3 or 7, doubled, < 100, halved):" << std::endl;
        std::cout << "  Results: ";
        for (auto val : complexPipeline) {
            std::cout << std::fixed << std::setprecision(1) << val << " ";
        }
        std::cout << std::endl;
        
        // Demonstrate function object reusability in different contexts
        std::cout << "\nFunction object reusability:" << std::endl;
        
        auto pipeline1 = dataSource | views::filter(multiplesOf3) | views::take(5);
        auto pipeline2 = dataSource | views::filter(multiplesOf7) | views::take(5);
        
        std::cout << "  Multiples of 3: ";
        for (auto val : pipeline1) std::cout << val << " ";
        std::cout << std::endl;
        
        std::cout << "  Multiples of 7: ";
        for (auto val : pipeline2) std::cout << val << " ";
        std::cout << std::endl;
    }
    
    void demonstrateRangesPerformance() {
        std::cout << "\n=== RANGES PERFORMANCE COMPARISON ===" << std::endl;
        
        // Generate larger dataset for performance testing
        std::vector<int> largeData;
        for (int i = 1; i <= 100000; ++i) {
            largeData.push_back(i);
        }
        
        auto measureTime = [](auto&& func) {
            auto start = std::chrono::high_resolution_clock::now();
            func();
            auto end = std::chrono::high_resolution_clock::now();
            return std::chrono::duration_cast<std::chrono::microseconds>(end - start).count() / 1000.0;
        };
        
        IsMultipleOf multiplesOf5(5);
        ScaleValue scaler(2.0);
        
        // Traditional approach timing
        double traditionalTime = measureTime([&]() {
            std::vector<int> filtered;
            std::copy_if(largeData.begin(), largeData.end(), 
                        std::back_inserter(filtered), multiplesOf5);
            
            std::vector<double> scaled(filtered.size());
            std::transform(filtered.begin(), filtered.end(), scaled.begin(), scaler);
            
            auto result = std::accumulate(scaled.begin(), scaled.begin() + std::min(1000ul, scaled.size()), 0.0);
            volatile double sink = result; // Prevent optimization
        });
        
        // Ranges approach timing
        double rangesTime = measureTime([&]() {
            auto pipeline = largeData
                | views::filter(multiplesOf5)
                | views::transform(scaler)
                | views::take(1000);
            auto common_pipeline = pipeline | std::views::common;
            auto result = std::accumulate(common_pipeline.begin(), common_pipeline.end(), 0.0);
            volatile double sink = result; // Prevent optimization
        });
        
        std::cout << "Performance comparison (100K elements):" << std::endl;
        std::cout << "  Traditional approach: " << std::fixed << std::setprecision(3) 
                  << traditionalTime << " ms" << std::endl;
        std::cout << "  Ranges approach: " << rangesTime << " ms" << std::endl;
        std::cout << "  Performance ratio: " << (traditionalTime / rangesTime) << "x" << std::endl;
        
        std::cout << "\nRanges advantages:" << std::endl;
        std::cout << "• Lazy evaluation reduces memory allocations" << std::endl;
        std::cout << "• Single-pass processing when possible" << std::endl;
        std::cout << "• No intermediate containers needed" << std::endl;
        std::cout << "• More expressive and composable code" << std::endl;
    }
    #else
    void demonstrateNoRangesAvailable() {
        std::cout << "\n=== C++20 RANGES NOT AVAILABLE ===" << std::endl;
        std::cout << "Your compiler doesn't support C++20 ranges." << std::endl;
        std::cout << "Traditional STL approach demonstrated above shows the concepts." << std::endl;
        std::cout << "Function objects work the same way with both approaches!" << std::endl;
    }
    #endif
    
    void demonstrateFunctionObjectAdvantages() {
        std::cout << "\n=== FUNCTION OBJECT ADVANTAGES ===" << std::endl;
        
        // Compare function objects with lambda expressions
        std::cout << "Function Objects vs. Lambda Expressions:" << std::endl;
        
        // Function object approach (reusable)
        IsMultipleOf multiplesOf6(6);
        ScaleValue tripler(3.0);
        
        auto count1 = std::count_if(dataSource.begin(), dataSource.end(), multiplesOf6);
        
        std::vector<double> scaled1(dataSource.size());
        std::transform(dataSource.begin(), dataSource.end(), scaled1.begin(), tripler);
        
        // Lambda approach (inline)
        auto count2 = std::count_if(dataSource.begin(), dataSource.end(), 
                                   [](int x) { return x % 6 == 0; });
        
        std::vector<double> scaled2(dataSource.size());
        std::transform(dataSource.begin(), dataSource.end(), scaled2.begin(),
                      [](int x) { return x * 3.0; });
        
        std::cout << "  Results identical: count = " << count1 << std::endl;
        std::cout << "  Function objects provide:" << std::endl;
        std::cout << "    ✓ Reusability across multiple contexts" << std::endl;
        std::cout << "    ✓ Named, self-documenting code" << std::endl;
        std::cout << "    ✓ State management capabilities" << std::endl;
        std::cout << "    ✓ Easy unit testing" << std::endl;
        std::cout << "  Lambda expressions provide:" << std::endl;
        std::cout << "    ✓ Concise inline definitions" << std::endl;
        std::cout << "    ✓ Capture of local variables" << std::endl;
        std::cout << "    ✓ Quick one-off operations" << std::endl;
    }
};

int main() {
    std::cout << "MODERN C++20 RANGES INTEGRATION WITH ITERATORS AND FUNCTION OBJECTS" << std::endl;
    std::cout << "===================================================================" << std::endl;
    
    RangeIntegration integration;
    
    // Generate test data
    integration.generateTestData();
    
    // Demonstrate different approaches
    integration.demonstrateTraditionalApproach();
    
    #if HAS_RANGES
    integration.demonstrateRangesApproach();
    integration.demonstrateRangesWithFunctionObjects();
    integration.demonstrateRangesPerformance();
    #else
    integration.demonstrateNoRangesAvailable();
    #endif
    
    integration.demonstrateFunctionObjectAdvantages();
    
    return 0;
}