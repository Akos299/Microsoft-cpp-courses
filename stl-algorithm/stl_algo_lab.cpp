// ============================================================================
// STL Algorithm Pipeline for Data Processing - Starter Files
// Overview: Students build a data processing pipeline using STL algorithms
//           with function objects and simple predicates for sales analysis
// Duration: 60-75 minutes
// Instructions: Follow the GRADED CHALLENGES to implement algorithm-based
//               data filtering, transformation, and analysis
// ============================================================================

/*
 * File: algorithm_pipeline.cpp
 * Author: [SEWANOU LEODASCE]
 * Date: [17/10/12]
 * Purpose: This program implements a data processing pipeline using STL algorithms
 *          with function objects for sales transaction analysis and transformation.
 */

#include <iostream>
#include <vector>
#include <algorithm>
#include <numeric>
#include <string>
#include <random>
#include <iomanip>

using namespace std;

// Constants
const int DATASET_SIZE = 1000;
const double TAX_RATE = 0.08;
const double DISCOUNT_THRESHOLD = 1000.0;
const double DISCOUNT_RATE = 0.10;

struct Transaction {
    int id;
    string customerId;
    double amount;
    string region;
    
    // Derived fields (to be calculated)
    double taxAmount;
    double discount;
    double finalTotal;
    
    Transaction(int i, string cust, double amt, string reg)
        : id(i), customerId(cust), amount(amt), region(reg),
          taxAmount(0), discount(0), finalTotal(amt) {}
};

// TODO: Function objects for validation and processing
// These replace complex lambda expressions with simple, readable classes

class IsValidTransaction {
public:
    bool operator()(const Transaction& t) const {
        if(!t.customerId.empty() && t.amount > 0.0)
            return true;
        return false; // Placeholder - replace with actual validation logic
    }
};

class IsHighValueTransaction {
public:
    bool operator()(const Transaction& t) const {
        // TODO: Return true if finalTotal > 500
        return t.finalTotal > 500;
    }
};

// TODO: Transformation function object
class CalculateDerivedValues {
public:
    Transaction operator()(Transaction t) const {
        t.taxAmount = t.amount * TAX_RATE;
        t.discount = (t.amount > DISCOUNT_THRESHOLD) ? t.amount * DISCOUNT_RATE : 0;
        t.finalTotal = t.amount + t.taxAmount - t.discount;
        return t; // Placeholder - return modified transaction
    }
};

// TODO: Simple helper functions (alternatives to complex function objects)
bool hasValidAmount(const Transaction& t) {
    // TODO: Return true if amount > 0
    return t.amount > 0;
}

bool hasValidCustomer(const Transaction& t) {
    // TODO: Return true if customerId is not empty
    return !t.customerId.empty();
}

double extractFinalTotal(const Transaction& t) {
    // TODO: Return the finalTotal field
    return t.finalTotal; // Placeholder
}

// TODO: Comparison functions for sorting
bool compareByFinalTotal(const Transaction& a, const Transaction& b) {
    // TODO: Return true if a.finalTotal > b.finalTotal (descending order)
    return (a.finalTotal > b.finalTotal);
}

bool compareByRegionThenAmount(const Transaction& a, const Transaction& b) {
    // TODO: Compare by region first, then by finalTotal if regions are equal
        if (a.region != b.region) {
        return a.region < b.region; // Sort regions alphabetically
    }
    return a.finalTotal > b.finalTotal; // Higher amounts first within region
}

class AlgorithmPipeline {
private:
    vector<Transaction> rawTransactions;
    vector<Transaction> validTransactions;
    
public:
    /**
     * GRADED CHALLENGE 1
     * TASK: Implement filterValidTransactions using STL algorithms with function objects
     * to filter out invalid transactions and count statistics.
     * 
     * Requirements:
     * - Use copy_if with IsValidTransaction function object
     * - Store results in validTransactions vector
     * - Count and display valid vs invalid transaction statistics
     * - Use count_if with appropriate predicates
     */
    void filterValidTransactions() {
        cout << "Original transactions: " << rawTransactions.size() << endl;
        
        //  Create IsValidTransaction function object
        IsValidTransaction isValid;
        
        //  Use copy_if to filter valid transactions
        validTransactions.clear();
        copy_if(rawTransactions.begin(), rawTransactions.end(), back_inserter(validTransactions), isValid);
        
        //  Count invalid transactions using count_if
        // You can create a "not valid" predicate or count and subtract
        auto invalid_transactions = rawTransactions.size() - count_if(rawTransactions.begin(), rawTransactions.end(), isValid);
        //  Display statistics
        cout << "Valid transactions: " << validTransactions.size() << endl;
        cout << "Invalid transactions: " << invalid_transactions << endl;
    }
    
    /**
     * GRADED CHALLENGE 2
     * TASK: Implement calculateDerivedValues using std::transform with function objects
     * to compute tax amounts, discounts, and final totals.
     * 
     * Requirements:
     * - Use std::transform with CalculateDerivedValues function object
     * - Transform validTransactions in-place
     * - Display sample calculation results
     * - All derived fields should be properly calculated
     */
    void calculateDerivedValues() {
        //Create CalculateDerivedValues function object
        CalculateDerivedValues calculator;
        
        // Use std::transform to calculate derived values in-place
        transform(validTransactions.begin(), validTransactions.end(),validTransactions.begin(), calculator);
        
        cout << "Calculated derived values for " << validTransactions.size() << " transactions" << endl;
        
        // Display sample calculation for first transaction
        if (!validTransactions.empty()) {
            const auto& sample = validTransactions[0];
            cout << "Sample: Amount $" << fixed << setprecision(2) << sample.amount;
            cout << " -> Final $" << sample.finalTotal << endl;
        }
    }
    
    /**
     * GRADED CHALLENGE 3
     * TASK: Implement generateStatistics using accumulate, count_if, and minmax_element
     * for basic data analysis with simple helper functions.
     * 
     * Requirements:
     * - Use std::accumulate to calculate total revenue
     * - Use std::count_if with IsHighValueTransaction for segmentation
     * - Use std::minmax_element to find highest/lowest transactions
     * - Calculate and display average transaction value
     * - Use simple helper functions instead of complex expressions
     */
    void generateStatistics() {
        cout << "\n=== Transaction Statistics ===" << endl;
        
        //  Calculate total revenue using std::accumulate
        // You can use a simple function or the extractFinalTotal helper

        double totalRevenue = accumulate(validTransactions.begin(), validTransactions.end(), 0.0,[](double acc, Transaction& t){
            return acc + extractFinalTotal(t);
        });
        
        // Count high-value transactions using count_if
        IsHighValueTransaction isHighValue;
        int highValueCount = count_if(validTransactions.begin(), validTransactions.end(), isHighValue);
        
        // Find min and max transactions using minmax_element
        // Use compareByFinalTotal or create a simple comparison
        auto [minTransaction, maxTransaction] = minmax_element(validTransactions.begin(), validTransactions.end(), compareByFinalTotal);
        
        // Calculate average
        double average = totalRevenue / validTransactions.size();
        
        // Display formatted results
        cout << fixed << setprecision(2);
        cout << "Total Revenue: $" << totalRevenue  << endl;
        cout << "Average Transaction: $" << average  << endl;
        cout << "Min Transaction: $" << minTransaction->finalTotal << endl;
        cout << "Max Transaction: $" << maxTransaction->finalTotal << endl;
        cout << "High-Value Transactions (>$500): " <<  highValueCount  << endl;
    }
    
    /**
     * GRADED CHALLENGE 4
     * TASK: Implement sortAndRank using sorting algorithms with simple comparison functions
     * to organize transactions and create rankings.
     * 
     * Requirements:
     * - Sort transactions by finalTotal using std::sort with compareByFinalTotal
     * - Use std::partial_sort to find top 10 transactions efficiently
     * - Sort by region then amount using compareByRegionThenAmount
     * - Display top transactions and regional rankings
     */
    void sortAndRank() {
        cout << "\n=== Sorting and Ranking Analysis ===" << endl;
        
        // Create copy for sorting operations
        vector<Transaction> sortedTransactions = validTransactions;
        
        //  Sort by finalTotal (descending) using std::sort
        sort(sortedTransactions.begin(), sortedTransactions.end(), compareByFinalTotal);
        
        // Display top 5 transactions
        cout << "Top 5 Transactions:" << endl;
        for (size_t i = 0; i < min(5ul, sortedTransactions.size()); ++i) {
            // Display transaction info: Customer ID, final total, region
            cout << sortedTransactions[i].customerId << ", "
                << sortedTransactions[i].finalTotal <<", "
                << sortedTransactions[i].region << "." << endl;
        }
        
        // TODO: Use partial_sort to find top 10 transactions efficiently
        vector<Transaction> topTransactions = validTransactions;
        partial_sort(topTransactions.begin(), topTransactions.begin() + min(10ul,topTransactions.size()),topTransactions.end(),compareByFinalTotal);

        
        // TODO: Sort by region then amount using compareByRegionThenAmount
        sort(sortedTransactions.begin(), sortedTransactions.end(), 
             compareByRegionThenAmount);
        
        cout << "Multi-criteria sorting completed" << endl;
    }
    
    /**
     * Function: generateTestData
     * Description: Creates realistic transaction data for algorithm processing
     */
    void generateTestData(int count) {
        random_device rd;
        mt19937 gen(rd());
        uniform_real_distribution<> amountDist(10.0, 2000.0);
        uniform_int_distribution<> customerDist(1, 100);
        
        vector<string> regions = {"North", "South", "East", "West"};
        
        for (int i = 0; i < count; ++i) {
            // Occasionally create invalid data for filtering practice
            double amount = (i % 50 == 0) ? -1.0 : amountDist(gen);
            string customerId = (i % 75 == 0) ? "" : "CUST_" + to_string(customerDist(gen));
            
            rawTransactions.emplace_back(
                i,
                customerId,
                amount,
                regions[i % regions.size()]
            );
        }
    }
    
    /**
     * Function: runPipelineTests
     * Description: Executes all pipeline stages and demonstrates algorithm usage
     */
    void runPipelineTests() {
        cout << "=== STL Algorithm Pipeline Processing ===" << endl;
        
        // Generate test data
        cout << "Generating test dataset..." << endl;
        generateTestData(DATASET_SIZE);
        cout << "✓ Generated " << rawTransactions.size() << " transactions" << endl;
        
        // Stage 1: Data filtering
        cout << "\nStage 1: Data validation and filtering..." << endl;
        filterValidTransactions();
        cout << "✓ Filtering complete" << endl;
        
        // Stage 2: Data transformation
        cout << "\nStage 2: Calculating derived values..." << endl;
        calculateDerivedValues();
        cout << "✓ Transformation complete" << endl;
        
        // Stage 3: Statistical analysis
        cout << "\nStage 3: Statistical analysis..." << endl;
        generateStatistics();
        cout << "✓ Statistics complete" << endl;
        
        // Stage 4: Sorting and ranking
        cout << "\nStage 4: Sorting and ranking..." << endl;
        sortAndRank();
        cout << "✓ Sorting complete" << endl;
    }
};

/*
REQUIREMENTS SUMMARY:
- Use function objects (classes with operator()) instead of complex expressions
- Apply copy_if and count_if for data filtering and counting
- Use std::transform with function objects for value calculations
- Use accumulate, minmax_element for statistical analysis with simple helpers
- Apply sorting algorithms with simple comparison functions
- Handle edge cases and display meaningful results
- Follow proper C++ coding standards and formatting
*/

/**
 * Function: main
 * Description: Program entry point demonstrating STL algorithm pipeline processing
 */
int main() {
    cout << "=== STL Algorithm Pipeline for Data Processing ===" << endl;
    cout << "Demonstrating algorithm usage with function objects and simple predicates\n" << endl;
    
    AlgorithmPipeline pipeline;
    pipeline.runPipelineTests();
    
    cout << "\n=== Pipeline Processing Complete ===" << endl;
    cout << "Review your algorithm choices and function object implementations!" << endl;
    
    return 0;
}

/*
EXPECTED OUTPUT STRUCTURE:
- Transaction counts and validation statistics using function objects
- Derived value calculations with std::transform and calculation objects  
- Statistical analysis using accumulate, count_if with simple predicates
- Sorted rankings using comparison functions
- Clear, formatted output showing algorithm effectiveness with readable code
*/