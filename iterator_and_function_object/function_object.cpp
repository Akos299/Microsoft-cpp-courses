#include <iostream>
#include <vector>
#include <algorithm>
#include <functional>
#include <numeric>
#include <string>
#include <map>
#include <iomanip>
#include <random>

// Basic function objects for data processing
class IsInRange {
private:
    int minValue, maxValue;
    
public:
    IsInRange(int min, int max) : minValue(min), maxValue(max) {}
    
    bool operator()(int value) const {
        return value >= minValue && value <= maxValue;
    }
    
    // Additional utility methods
    int getMin() const { return minValue; }
    int getMax() const { return maxValue; }
    void setRange(int min, int max) { minValue = min; maxValue = max; }
};

class StatisticalTransform {
private:
    double mean;
    double stdDev;
    
public:
    StatisticalTransform(double m, double sd) : mean(m), stdDev(sd) {}
    
    double operator()(int value) const {
        // Z-score normalization: (value - mean) / stdDev
        return (value - mean) / stdDev;
    }
    
    void updateStatistics(double newMean, double newStdDev) {
        mean = newMean;
        stdDev = newStdDev;
    }
};

class ConditionalAccumulator {
private:
    int threshold;
    std::string operation; // "above", "below", "equal"
    
public:
    ConditionalAccumulator(int thresh, const std::string& op) 
        : threshold(thresh), operation(op) {}
    
    int operator()(int accumulator, int value) const {
        if (operation == "above" && value > threshold) {
            return accumulator + value;
        } else if (operation == "below" && value < threshold) {
            return accumulator + value;
        } else if (operation == "equal" && value == threshold) {
            return accumulator + value;
        }
        return accumulator;
    }
};

// Advanced function object with complex state
class FrequencyCounter {
private:
    mutable std::map<int, int> frequencies;
    int totalCount;
    
public:
    FrequencyCounter() : totalCount(0) {}
    
    void operator()(int value) const {
        frequencies[value]++;
        const_cast<FrequencyCounter*>(this)->totalCount++;
    }
    
    // Query methods
    int getFrequency(int value) const {
        auto it = frequencies.find(value);
        return (it != frequencies.end()) ? it->second : 0;
    }
    
    double getRelativeFrequency(int value) const {
        return totalCount > 0 ? static_cast<double>(getFrequency(value)) / totalCount : 0.0;
    }
    
    std::vector<std::pair<int, int>> getMostFrequent(int topN = 5) const {
        std::vector<std::pair<int, int>> sorted(frequencies.begin(), frequencies.end());
        
        std::partial_sort(sorted.begin(), 
                         sorted.begin() + std::min(topN, static_cast<int>(sorted.size())),
                         sorted.end(),
                         [](const auto& a, const auto& b) {
                             return a.second > b.second;
                         });
        
        sorted.resize(std::min(topN, static_cast<int>(sorted.size())));
        return sorted;
    }
    
    void reset() {
        frequencies.clear();
        totalCount = 0;
    }
};

class FunctionObjectDesign {
private:
    std::vector<int> dataset;
    
public:
    void generateDataset() {
        dataset.clear();
        
        // Create a dataset with interesting statistical properties
        std::random_device rd;
        std::mt19937 gen(rd());
        
        // Mix of different distributions
        std::normal_distribution<double> normal(50.0, 15.0);
        std::uniform_int_distribution<int> uniform(1, 100);
        
        for (int i = 0; i < 5000; ++i) {
            if (i < 3000) {
                // Majority normal distribution
                int value = static_cast<int>(std::max(1.0, std::min(100.0, normal(gen))));
                dataset.push_back(value);
            } else {
                // Some uniform values
                dataset.push_back(uniform(gen));
            }
        }
        
        // Add some specific values for frequency analysis
        for (int i = 0; i < 100; ++i) {
            dataset.push_back(42); // Will be frequent
        }
        for (int i = 0; i < 50; ++i) {
            dataset.push_back(77); // Will be somewhat frequent
        }
        
        std::cout << "Generated dataset with " << dataset.size() << " elements" << std::endl;
    }
    
    void demonstrateBasicFunctionObjects() {
        std::cout << "\n=== BASIC FUNCTION OBJECT DEMONSTRATIONS ===" << std::endl;
        
        // Demonstrate IsInRange function object
        std::cout << "Range Filtering Analysis:" << std::endl;
        
        IsInRange normalRange(25, 75);
        IsInRange lowRange(1, 25);
        IsInRange highRange(75, 100);
        
        int normalCount = std::count_if(dataset.begin(), dataset.end(), normalRange);
        int lowCount = std::count_if(dataset.begin(), dataset.end(), lowRange);
        int highCount = std::count_if(dataset.begin(), dataset.end(), highRange);
        
        std::cout << "  Normal range (25-75): " << normalCount << " elements ("
                  << std::fixed << std::setprecision(1) 
                  << 100.0 * normalCount / dataset.size() << "%)" << std::endl;
        std::cout << "  Low range (1-25): " << lowCount << " elements ("
                  << 100.0 * lowCount / dataset.size() << "%)" << std::endl;
        std::cout << "  High range (75-100): " << highCount << " elements ("
                  << 100.0 * highCount / dataset.size() << "%)" << std::endl;
        
        // Demonstrate reusability by changing range
        normalRange.setRange(30, 70);
        int newNormalCount = std::count_if(dataset.begin(), dataset.end(), normalRange);
        std::cout << "  Adjusted range (30-70): " << newNormalCount << " elements" << std::endl;
    }
    
    void demonstrateStatisticalTransformation() {
        std::cout << "\n=== STATISTICAL TRANSFORMATION FUNCTION OBJECT ===" << std::endl;
        
        // Calculate dataset statistics
        double sum = std::accumulate(dataset.begin(), dataset.end(), 0.0);
        double mean = sum / dataset.size();
        
        double variance = std::accumulate(dataset.begin(), dataset.end(), 0.0,
            [mean](double acc, int val) {
                double diff = val - mean;
                return acc + diff * diff;
            }) / dataset.size();
        
        double stdDev = std::sqrt(variance);
        
        std::cout << "Dataset Statistics:" << std::endl;
        std::cout << "  Mean: " << std::fixed << std::setprecision(2) << mean << std::endl;
        std::cout << "  Standard Deviation: " << stdDev << std::endl;
        
        // Create and use statistical transformation
        StatisticalTransform normalizer(mean, stdDev);
        
        std::vector<double> normalizedData(dataset.size());
        std::transform(dataset.begin(), dataset.end(), normalizedData.begin(), normalizer);
        
        // Analyze normalized data
        double normalizedMean = std::accumulate(normalizedData.begin(), normalizedData.end(), 0.0) 
                               / normalizedData.size();
        
        auto [minIt, maxIt] = std::minmax_element(normalizedData.begin(), normalizedData.end());
        
        std::cout << "Normalized Data Analysis:" << std::endl;
        std::cout << "  Normalized mean: " << normalizedMean << " (should be ≈0)" << std::endl;
        std::cout << "  Range: [" << *minIt << ", " << *maxIt << "]" << std::endl;
        
        // Count values within standard deviations
        int oneStdDev = std::count_if(normalizedData.begin(), normalizedData.end(),
            [](double val) { return std::abs(val) <= 1.0; });
        int twoStdDev = std::count_if(normalizedData.begin(), normalizedData.end(),
            [](double val) { return std::abs(val) <= 2.0; });
        
        std::cout << "  Within 1 std dev: " << oneStdDev << " (" 
                  << std::setprecision(1) << 100.0 * oneStdDev / dataset.size() << "%)" << std::endl;
        std::cout << "  Within 2 std dev: " << twoStdDev << " (" 
                  << 100.0 * twoStdDev / dataset.size() << "%)" << std::endl;
    }
    
    void demonstrateConditionalAccumulation() {
        std::cout << "\n=== CONDITIONAL ACCUMULATION FUNCTION OBJECT ===" << std::endl;
        
        // Create different conditional accumulators
        ConditionalAccumulator sumAbove50(50, "above");
        ConditionalAccumulator sumBelow30(30, "below");
        ConditionalAccumulator sumEquals42(42, "equal");
        
        int totalAbove50 = std::accumulate(dataset.begin(), dataset.end(), 0, sumAbove50);
        int totalBelow30 = std::accumulate(dataset.begin(), dataset.end(), 0, sumBelow30);
        int totalEquals42 = std::accumulate(dataset.begin(), dataset.end(), 0, sumEquals42);
        
        std::cout << "Conditional Accumulation Results:" << std::endl;
        std::cout << "  Sum of values > 50: " << totalAbove50 << std::endl;
        std::cout << "  Sum of values < 30: " << totalBelow30 << std::endl;
        std::cout << "  Sum of values = 42: " << totalEquals42 << std::endl;
        
        // Calculate percentages of total sum
        int totalSum = std::accumulate(dataset.begin(), dataset.end(), 0);
        std::cout << "  Percentage of total sum:" << std::endl;
        std::cout << "    Above 50: " << std::fixed << std::setprecision(1) 
                  << 100.0 * totalAbove50 / totalSum << "%" << std::endl;
        std::cout << "    Below 30: " << 100.0 * totalBelow30 / totalSum << "%" << std::endl;
        std::cout << "    Equals 42: " << 100.0 * totalEquals42 / totalSum << "%" << std::endl;
    }
    
    void demonstrateAdvancedFunctionObject() {
        std::cout << "\n=== ADVANCED FUNCTION OBJECT WITH COMPLEX STATE ===" << std::endl;
        
        // Use frequency counter function object
        FrequencyCounter counter;
        
        // Process all data
        std::for_each(dataset.begin(), dataset.end(), counter);
        
        std::cout << "Frequency Analysis Results:" << std::endl;
        
        // Show most frequent values
        auto mostFrequent = counter.getMostFrequent(10);
        std::cout << "  Top 10 most frequent values:" << std::endl;
        for (const auto& [value, freq] : mostFrequent) {
            std::cout << "    Value " << value << ": " << freq << " times ("
                      << std::fixed << std::setprecision(2) 
                      << counter.getRelativeFrequency(value) * 100 << "%)" << std::endl;
        }
        
        // Query specific values
        std::cout << "  Specific value frequencies:" << std::endl;
        for (int testValue : {42, 77, 50, 1, 100}) {
            int freq = counter.getFrequency(testValue);
            if (freq > 0) {
                std::cout << "    Value " << testValue << ": " << freq << " times" << std::endl;
            }
        }
    }
    
    void demonstrateFunctionObjectComposition() {
        std::cout << "\n=== FUNCTION OBJECT COMPOSITION ===" << std::endl;
        
        // Combine multiple function objects in a processing pipeline
        std::cout << "Multi-stage Processing Pipeline:" << std::endl;
        
        // Stage 1: Filter values in normal range
        IsInRange normalRange(30, 70);
        std::vector<int> filteredData;
        std::copy_if(dataset.begin(), dataset.end(), std::back_inserter(filteredData), normalRange);
        
        std::cout << "  Stage 1 - Filtered to normal range: " << filteredData.size() 
                  << " elements" << std::endl;
        
        // Stage 2: Statistical transformation
        double mean = std::accumulate(filteredData.begin(), filteredData.end(), 0.0) / filteredData.size();
        double variance = std::accumulate(filteredData.begin(), filteredData.end(), 0.0,
            [mean](double acc, int val) {
                double diff = val - mean;
                return acc + diff * diff;
            }) / filteredData.size();
        double stdDev = std::sqrt(variance);
        
        StatisticalTransform normalizer(mean, stdDev);
        std::vector<double> normalizedData(filteredData.size());
        std::transform(filteredData.begin(), filteredData.end(), normalizedData.begin(), normalizer);
        
        std::cout << "  Stage 2 - Statistical normalization applied" << std::endl;
        
        // Stage 3: Count extreme values (|z| > 2)
        int extremeCount = std::count_if(normalizedData.begin(), normalizedData.end(),
            [](double val) { return std::abs(val) > 2.0; });
        
        std::cout << "  Stage 3 - Extreme values (|z| > 2): " << extremeCount << std::endl;
        
        // Stage 4: Final frequency analysis of original filtered data
        FrequencyCounter finalCounter;
        std::for_each(filteredData.begin(), filteredData.end(), finalCounter);
        
        auto topValues = finalCounter.getMostFrequent(5);
        std::cout << "  Stage 4 - Top 5 values in filtered data:" << std::endl;
        for (const auto& [value, freq] : topValues) {
            std::cout << "    " << value << " (" << freq << " times)" << std::endl;
        }
        
        std::cout << "Pipeline demonstrates function object reusability and composition!" << std::endl;
    }
};

int main() {
    std::cout << "CUSTOM FUNCTION OBJECT DESIGN AND COMPOSITION" << std::endl;
    std::cout << "==============================================" << std::endl;
    
    FunctionObjectDesign design;
    
    // Generate test dataset
    design.generateDataset();
    
    // Demonstrate different aspects of function object design
    design.demonstrateBasicFunctionObjects();
    design.demonstrateStatisticalTransformation();
    design.demonstrateConditionalAccumulation();
    design.demonstrateAdvancedFunctionObject();
    design.demonstrateFunctionObjectComposition();
    
    return 0;
}