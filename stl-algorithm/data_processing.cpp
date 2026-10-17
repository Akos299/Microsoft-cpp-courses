#include <iostream>
#include <vector>
#include <algorithm>
#include <numeric>
#include <functional>
#include <map>
#include <unordered_map>
#include <string>
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

struct SalesData {
    std::string region;
    std::string product;
    double revenue;
    int units;
    std::string month;
    
    SalesData(const std::string& r, const std::string& p, double rev, int u, const std::string& m)
        : region(r), product(p), revenue(rev), units(u), month(m) {}
    
    double pricePerUnit() const {
        return units > 0 ? revenue / units : 0.0;
    }
};

class AlgorithmPipeline {
private:
    std::vector<SalesData> salesData;
    
public:
    void generateSalesData() {
        std::vector<std::string> regions = {"North", "South", "East", "West"};
        std::vector<std::string> products = {"Laptop", "Phone", "Tablet", "Watch", "Headphones"};
        std::vector<std::string> months = {"Jan", "Feb", "Mar", "Apr", "May", "Jun"};
        
        salesData.clear();
        
        // Generate comprehensive sales data
        for (const auto& region : regions) {
            for (const auto& product : products) {
                for (const auto& month : months) {
                    // Simulate varying sales performance
                    double baseRevenue = 1000.0 + (std::hash<std::string>{}(region + product) % 5000);
                    int units = 10 + (std::hash<std::string>{}(product + month) % 50);
                    double revenue = baseRevenue + (std::hash<std::string>{}(month) % 2000);
                    
                    salesData.emplace_back(region, product, revenue, units, month);
                }
            }
        }
        
        std::cout << "Generated " << salesData.size() << " sales records" << std::endl;
    }
    
    void traditionalAnalysisPipeline() {
        std::cout << "\n=== TRADITIONAL STL PIPELINE ANALYSIS ===" << std::endl;
        
        // Pipeline Step 1: Filter high-revenue sales (>$3000)
        std::vector<SalesData> highRevenueSales;
        std::copy_if(salesData.begin(), salesData.end(), 
                    std::back_inserter(highRevenueSales),
                    [](const SalesData& s) { return s.revenue > 3000.0; });
        
        std::cout << "Step 1 - High revenue sales: " << highRevenueSales.size() 
                  << " out of " << salesData.size() << std::endl;
        
        // Pipeline Step 2: Transform to calculate price per unit
        std::vector<double> pricesPerUnit;
        std::transform(highRevenueSales.begin(), highRevenueSales.end(),
                      std::back_inserter(pricesPerUnit),
                      [](const SalesData& s) { return s.pricePerUnit(); });
        
        std::cout << "Step 2 - Price per unit calculated for " << pricesPerUnit.size() 
                  << " records" << std::endl;
        
        // Pipeline Step 3: Calculate statistics
        double totalRevenue = std::accumulate(highRevenueSales.begin(), highRevenueSales.end(), 0.0,
            [](double sum, const SalesData& s) { return sum + s.revenue; });
        
        double avgPricePerUnit = std::accumulate(pricesPerUnit.begin(), pricesPerUnit.end(), 0.0) 
                                / pricesPerUnit.size();
        
        auto [minPrice, maxPrice] = std::minmax_element(pricesPerUnit.begin(), pricesPerUnit.end());
        
        std::cout << "Step 3 - Statistics calculated:" << std::endl;
        std::cout << "  Total high-revenue sales: $" << std::fixed << std::setprecision(2) 
                  << totalRevenue << std::endl;
        std::cout << "  Average price per unit: $" << avgPricePerUnit << std::endl;
        std::cout << "  Price range: $" << *minPrice << " - $" << *maxPrice << std::endl;
        
        // Pipeline Step 4: Group by region and find top performer
        std::map<std::string, double> regionTotals;
        std::for_each(highRevenueSales.begin(), highRevenueSales.end(),
            [&regionTotals](const SalesData& s) {
                regionTotals[s.region] += s.revenue;
            });
        
        auto topRegion = std::max_element(regionTotals.begin(), regionTotals.end(),
            [](const auto& a, const auto& b) {
                return a.second < b.second;
            });
        
        std::cout << "Step 4 - Top performing region: " << topRegion->first 
                  << " with $" << topRegion->second << std::endl;
        
        // Pipeline Step 5: Sort and display top products
        std::vector<SalesData> sortedSales = highRevenueSales;
        std::sort(sortedSales.begin(), sortedSales.end(),
            [](const SalesData& a, const SalesData& b) {
                return a.revenue > b.revenue;
            });
        
        std::cout << "Step 5 - Top 3 high-revenue products:" << std::endl;
        for (int i = 0; i < std::min(3, static_cast<int>(sortedSales.size())); ++i) {
            const auto& s = sortedSales[i];
            std::cout << "  " << i+1 << ". " << s.product << " (" << s.region 
                      << ", " << s.month << "): $" << s.revenue << std::endl;
        }
    }
    
    #if HAS_RANGES
    void modernRangesPipeline() {
        std::cout << "\n=== C++20 RANGES PIPELINE ANALYSIS ===" << std::endl;
        
        // Compose entire pipeline in one expression
        auto highRevenueAnalysis = salesData 
            | views::filter([](const SalesData& s) { return s.revenue > 3000.0; })
            | views::transform([](const SalesData& s) { return s; }); // Keep full data for analysis
        
        // Convert to vector for further analysis (ranges are lazy)
        std::vector<SalesData> highRevenueSales(highRevenueAnalysis.begin(), highRevenueAnalysis.end());
        
        std::cout << "Pipeline filtered to " << highRevenueSales.size() << " high-revenue sales" << std::endl;
        
        // Create a composable pipeline for price analysis
        auto priceAnalysis = highRevenueSales
            | views::transform([](const SalesData& s) { return s.pricePerUnit(); })
            | views::filter([](double price) { return price > 0; }); // Filter valid prices
        
        // Calculate statistics using ranges
        auto priceVector = std::vector(priceAnalysis.begin(), priceAnalysis.end());
        double totalRevenue = std::accumulate(highRevenueSales.begin(), highRevenueSales.end(), 0.0,
            [](double sum, const SalesData& s) { return sum + s.revenue; });
        
        double avgPrice = std::accumulate(priceVector.begin(), priceVector.end(), 0.0) / priceVector.size();
        auto [minPrice, maxPrice] = rng::minmax_element(priceVector);
        
        std::cout << "Ranges pipeline results:" << std::endl;
        std::cout << "  Total revenue: $" << std::fixed << std::setprecision(2) << totalRevenue << std::endl;
        std::cout << "  Average price per unit: $" << avgPrice << std::endl;
        std::cout << "  Price range: $" << *minPrice << " - $" << *maxPrice << std::endl;
        
        // Demonstrate complex composition: Top products by region
        std::cout << "Top products by region using ranges:" << std::endl;
        
        std::vector<std::string> regions = {"North", "South", "East", "West"};
        for (const auto& region : regions) {
            auto topProductInRegion = highRevenueSales
                | views::filter([&region](const SalesData& s) { return s.region == region; })
                | views::transform([](const SalesData& s) { return std::make_pair(s.product, s.revenue); });
            
            if (!topProductInRegion.empty()) {
                auto maxProduct = rng::max_element(topProductInRegion, 
                [](const auto& a, const auto& b){ return a.second < b.second;});
                
                // std::cout << "  " << region << ": " << maxProduct->fisrt
std::cout << "  " << region << ": " << (*maxProduct).first  // for C++20
                        //   << " ($" << maxProduct->second << ")" << std::endl;
<< " ($" << (*maxProduct).second << ")" << std::endl; //for C++20

            }
        }
        
        // Demonstrate lazy evaluation with complex filtering
        std::cout << "Lazy evaluation demo - Complex multi-stage filter:" << std::endl;
        auto complexPipeline = salesData
            | views::filter([](const SalesData& s) { return s.units > 20; })
            | views::filter([](const SalesData& s) { return s.revenue > 2000; })
            | views::transform([](const SalesData& s) { 
                return s.product + " (" + s.region + "): $" + std::to_string(s.revenue); 
              })
            | views::take(5);
        
        for (const auto& result : complexPipeline) {
            std::cout << "  " << result << std::endl;
        }
    }
    #endif
    
    void performanceComparison() {
        std::cout << "\n=== PERFORMANCE AND READABILITY COMPARISON ===" << std::endl;
        
        auto measureExecutionTime = [](auto&& func) {
            auto start = std::chrono::high_resolution_clock::now();
            func();
            auto end = std::chrono::high_resolution_clock::now();
            return std::chrono::duration_cast<std::chrono::microseconds>(end - start).count() / 1000.0;
        };
        
        // Traditional approach timing
        double traditionalTime = measureExecutionTime([this]() {
            std::vector<SalesData> filtered;
            std::copy_if(salesData.begin(), salesData.end(), std::back_inserter(filtered),
                [](const SalesData& s) { return s.revenue > 3000.0 && s.units > 15; });
            
            std::vector<double> transformed;
            std::transform(filtered.begin(), filtered.end(), std::back_inserter(transformed),
                [](const SalesData& s) { return s.pricePerUnit(); });
            
            std::sort(transformed.begin(), transformed.end(), std::greater<double>());
        });
        
        std::cout << "Traditional STL pipeline: " << std::fixed << std::setprecision(3) 
                  << traditionalTime << " ms" << std::endl;
        
        #if HAS_RANGES
        // Ranges approach timing
        double rangesTime = measureExecutionTime([this]() {
            auto pipeline = salesData
                | views::filter([](const SalesData& s) { return s.revenue > 3000.0 && s.units > 15; })
                | views::transform([](const SalesData& s) { return s.pricePerUnit(); });
            
            auto result = std::vector(pipeline.begin(), pipeline.end());
            rng::sort(result, std::greater<double>());
        });
        
        std::cout << "C++20 Ranges pipeline: " << rangesTime << " ms" << std::endl;
        std::cout << "Performance ratio: " << (traditionalTime / rangesTime) << "x" << std::endl;
        
        std::cout << "\nReadability Analysis:" << std::endl;
        std::cout << "✓ Ranges: More natural left-to-right reading flow" << std::endl;
        std::cout << "✓ Ranges: Fewer intermediate variables needed" << std::endl;
        std::cout << "✓ Ranges: Lazy evaluation reduces memory allocations" << std::endl;
        std::cout << "✓ Traditional: More explicit control over memory and performance" << std::endl;
        std::cout << "✓ Traditional: Better debugging support in most toolchains" << std::endl;
        #endif
    }
};

int main(){
    std::cout << "STL ALGORITHM PIPELINE COMPOSITION ANALYSIS" << std::endl;
    std::cout << "===========================================" << std::endl;
    
    AlgorithmPipeline pipeline;
    
    // Generate test data
    pipeline.generateSalesData();
    
    // Demonstrate traditional approach
    pipeline.traditionalAnalysisPipeline();
    
    #if HAS_RANGES
    // Demonstrate modern ranges approach
    pipeline.modernRangesPipeline();
    #endif
    
    // Compare approaches
    pipeline.performanceComparison();
    
    return 0;
}