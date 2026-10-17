#include <iostream>
#include <vector>
#include <algorithm>
#include <numeric>
#include <random>
#include <chrono>
#include <string>
#include <iomanip>

#if __cplusplus >= 202002L
#include <ranges>
namespace rng = std::ranges;
namespace views = std::views;
#define HAS_RANGES 1
#else
#define HAS_RANGES 0
#endif

struct CustomerTransaction {
    int customerId;
    double amount;
    std::string category;
    int timestamp;
    
    CustomerTransaction(int id, double amt, const std::string& cat, int ts)
        : customerId(id), amount(amt), category(cat), timestamp(ts) {}
};

class AlgorithmComparison {
private:
    std::vector<CustomerTransaction> transactions;
    std::mt19937 rng{std::random_device{}()};
    
    template<typename Func>
    double measureTime(Func&& func) {
        auto start = std::chrono::high_resolution_clock::now();
        func();
        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
        return duration.count() / 1000.0; // Convert to milliseconds
    }
    
public:
    void generateTestData(int count) {
        transactions.clear();
        transactions.reserve(count);
        
        std::uniform_real_distribution<double> amountDist(10.0, 500.0);
        std::uniform_int_distribution<int> customerDist(1, 1000);
        std::uniform_int_distribution<int> timestampDist(1000000, 2000000);
        
        std::vector<std::string> categories = {"Electronics", "Clothing", "Books", "Food", "Home"};
        
        for (int i = 0; i < count; ++i) {
            transactions.emplace_back(
                customerDist(rng),
                amountDist(rng),
                categories[i % categories.size()],
                timestampDist(rng)
            );
        }
        
        std::cout << "Generated " << transactions.size() << " test transactions" << std::endl;
    }
    
    void demonstrateSearchAlgorithms() {
        std::cout << "\n=== SEARCH ALGORITHM DEMONSTRATIONS ===" << std::endl;
        
        // Generate target values for searching
        int targetCustomer = 500;
        double targetAmount = 250.0;
        
        // Traditional STL find
        std::cout << "Traditional STL Approaches:" << std::endl;
        
        double traditionalFindTime = measureTime([&]() {
            auto it = std::find_if(transactions.begin(), transactions.end(),
                [targetCustomer](const CustomerTransaction& t) {
                    return t.customerId == targetCustomer;
                });
            volatile bool found = (it != transactions.end());
        });
        
        // Traditional STL count_if for analysis
        int electronicsCount = 0;
        double traditionalCountTime = measureTime([&]() {
            electronicsCount = std::count_if(transactions.begin(), transactions.end(),
                [](const CustomerTransaction& t) {
                    return t.category == "Electronics";
                });
        });
        
        std::cout << "  find_if execution time: " << std::fixed << std::setprecision(3) 
                  << traditionalFindTime << " ms" << std::endl;
        std::cout << "  count_if execution time: " << traditionalCountTime << " ms" << std::endl;
        std::cout << "  Electronics transactions found: " << electronicsCount << std::endl;
        
        #if HAS_RANGES
        std::cout << "\nC++20 Ranges Approaches:" << std::endl;
        
        // Range-based find
        double rangeFindTime = measureTime([&]() {
            auto result = rng::find_if(transactions,
                [targetCustomer](const CustomerTransaction& t) {
                    return t.customerId == targetCustomer;
                });
            volatile bool found = (result != transactions.end());
        });
        
        // Range-based count
        int rangeElectronicsCount = 0;
        double rangeCountTime = measureTime([&]() {
            rangeElectronicsCount = rng::count_if(transactions,
                [](const CustomerTransaction& t) {
                    return t.category == "Electronics";
                });
        });
        
        std::cout << "  ranges::find_if execution time: " << rangeFindTime << " ms" << std::endl;
        std::cout << "  ranges::count_if execution time: " << rangeCountTime << " ms" << std::endl;
        std::cout << "  Electronics transactions found: " << rangeElectronicsCount << std::endl;
        
        std::cout << "\nPerformance Comparison:" << std::endl;
        std::cout << "  Find speedup with ranges: " 
                  << (traditionalFindTime > 0 ? traditionalFindTime / rangeFindTime : 1.0) << "x" << std::endl;
        std::cout << "  Count speedup with ranges: " 
                  << (traditionalCountTime > 0 ? traditionalCountTime / rangeCountTime : 1.0) << "x" << std::endl;
        #else
        std::cout << "\nC++20 Ranges not available in this compiler" << std::endl;
        #endif
    }
    
    void demonstrateTransformationAlgorithms() {
        std::cout << "\n=== TRANSFORMATION ALGORITHM DEMONSTRATIONS ===" << std::endl;
        
        // Create copy for transformations
        std::vector<CustomerTransaction> workingCopy = transactions;
        
        // Traditional STL transform
        std::cout << "Traditional STL transform:" << std::endl;
        
        std::vector<double> adjustedAmounts(workingCopy.size());
        double transformTime = measureTime([&]() {
            std::transform(workingCopy.begin(), workingCopy.end(), 
                          adjustedAmounts.begin(),
                          [](const CustomerTransaction& t) {
                              // Apply 10% discount
                              return t.amount * 0.9;
                          });
        });
        
        std::cout << "  transform execution time: " << transformTime << " ms" << std::endl;
        std::cout << "  Sample transformed values: ";
        for (int i = 0; i < std::min(5, static_cast<int>(adjustedAmounts.size())); ++i) {
            std::cout << std::fixed << std::setprecision(2) << adjustedAmounts[i] << " ";
        }
        std::cout << std::endl;
        
        #if HAS_RANGES
        std::cout << "\nC++20 ranges transform view:" << std::endl;
        
        double rangeTransformTime = measureTime([&]() {
            auto discountedView = workingCopy | views::transform([](const CustomerTransaction& t) {
                return t.amount * 0.9;
            });
            
            // Force evaluation by consuming the view
            volatile double sum = 0;
            for (auto value : discountedView | views::take(1000)) {
                sum += value;
            }
        });
        
        std::cout << "  ranges transform execution time: " << rangeTransformTime << " ms" << std::endl;
        
        // Demonstrate lazy evaluation advantage
        std::cout << "  Lazy evaluation demo:" << std::endl;
        auto lazyTransform = workingCopy | views::transform([](const CustomerTransaction& t) {
            return t.amount * 0.9;
        }) | views::filter([](double amount) {
            return amount > 100.0;
        }) | views::take(5);
        
        std::cout << "  First 5 discounted amounts > $100: ";
        for (auto amount : lazyTransform) {
            std::cout << std::fixed << std::setprecision(2) << amount << " ";
        }
        std::cout << std::endl;
        #endif
    }
    
    void demonstrateSortingAlgorithms() {
        std::cout << "\n=== SORTING ALGORITHM DEMONSTRATIONS ===" << std::endl;
        
        // Create copies for different sorting demonstrations
        std::vector<CustomerTransaction> sortByAmount = transactions;
        std::vector<CustomerTransaction> sortByCustomer = transactions;
        
        // Sort by amount (descending)
        double amountSortTime = measureTime([&]() {
            std::sort(sortByAmount.begin(), sortByAmount.end(),
                [](const CustomerTransaction& a, const CustomerTransaction& b) {
                    return a.amount > b.amount;
                });
        });
        
        // Sort by customer ID
        double customerSortTime = measureTime([&]() {
            std::sort(sortByCustomer.begin(), sortByCustomer.end(),
                [](const CustomerTransaction& a, const CustomerTransaction& b) {
                    return a.customerId < b.customerId;
                });
        });
        
        std::cout << "Traditional sorting results:" << std::endl;
        std::cout << "  Sort by amount time: " << amountSortTime << " ms" << std::endl;
        std::cout << "  Sort by customer time: " << customerSortTime << " ms" << std::endl;
        
        std::cout << "  Top 5 transactions by amount:" << std::endl;
        for (int i = 0; i < std::min(5, static_cast<int>(sortByAmount.size())); ++i) {
            const auto& t = sortByAmount[i];
            std::cout << "    Customer " << t.customerId << ": $" 
                      << std::fixed << std::setprecision(2) << t.amount 
                      << " (" << t.category << ")" << std::endl;
        }
        
        #if HAS_RANGES
        std::cout << "\nC++20 ranges sorting:" << std::endl;
        
        std::vector<CustomerTransaction> rangeSortCopy = transactions;
        double rangeSortTime = measureTime([&]() {
            rng::sort(rangeSortCopy, [](const CustomerTransaction& a, const CustomerTransaction& b) {
                return a.amount > b.amount;
            });
        });
        
        std::cout << "  ranges::sort time: " << rangeSortTime << " ms" << std::endl;
        std::cout << "  Performance comparison: " 
                  << (amountSortTime > 0 ? amountSortTime / rangeSortTime : 1.0) << "x" << std::endl;
        #endif
    }
};






int main(int argc, char* argv []){
    std::cout << "STL ALGORITHM APPLICATION AND PERFORMANCE COMPARISON" << std::endl;
    std::cout << "====================================================" << std::endl;
    
    AlgorithmComparison comparison;
    
    // Generate test data
    std::cout << "Generating test data..." << std::endl;
    int count = 50000; // Default value

    if (argc > 1) {
        count = std::stoi(argv[1]);
    }
    comparison.generateTestData(count);
    
    // Demonstrate different algorithm categories
    comparison.demonstrateSearchAlgorithms();
    comparison.demonstrateTransformationAlgorithms();
    comparison.demonstrateSortingAlgorithms();
    
    #if HAS_RANGES
    std::cout << "\n=== C++20 RANGES ADVANTAGES ===" << std::endl;
    std::cout << "✓ More readable and expressive syntax" << std::endl;
    std::cout << "✓ Composable operations with pipe syntax" << std::endl;
    std::cout << "✓ Lazy evaluation for better performance" << std::endl;
    std::cout << "✓ Built-in range safety and bounds checking" << std::endl;
    #else
    std::cout << "\n=== TRADITIONAL STL ADVANTAGES ===" << std::endl;
    std::cout << "✓ Broad compiler compatibility" << std::endl;
    std::cout << "✓ Well-established performance characteristics" << std::endl;
    std::cout << "✓ Extensive documentation and community knowledge" << std::endl;
    std::cout << "✓ Fine-grained control over memory and performance" << std::endl;
    #endif
    
    return 0;
}