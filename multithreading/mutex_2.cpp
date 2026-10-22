#include <functional>
#include <iostream>
#include <thread>
#include <mutex>
#include <vector>
#include <atomic>
#include <chrono>
#include <random>
#include <stdexcept>
#include <memory>


std::mutex dataMutex;
std::mutex outputMutex;
std::vector<std::vector<double>> sharedData;
std::atomic<int> processedCount{0};

void dataProcessor(int processorId, int itemCount) {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<> dis(1.0, 100.0);
    
    for (int i = 0; i < itemCount; ++i) {
        double value = dis(gen);
        
        // Thread-safe data insertion
        {
            std::lock_guard<std::mutex> lock(dataMutex);
            sharedData[processorId].push_back(value);
        }
        
        processedCount.fetch_add(1);
        
        // Simulate processing time
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    
    std::cout << "Processor " << processorId << " completed processing " 
              << itemCount << " items\n\n";
}



class ThreadGuard {
private:
    std::thread& thread_;
    
public:
    explicit ThreadGuard(std::thread& t) : thread_(t) {}
    
    ~ThreadGuard() {
        if (thread_.joinable()) {
            thread_.join();
        }
    }
    
    // Prevent copying
    ThreadGuard(const ThreadGuard&) = delete;
    ThreadGuard& operator=(const ThreadGuard&) = delete;
};

void riskAnalyzer(int analyzerId, const std::vector<double>& data, 
                  double& result) {
    try {
        if (data.empty()) {
            throw std::runtime_error("No data to analyze");
        }
        
        double sum = 0.0;
        for (double value : data) {
            sum += value * value;  // Risk calculation simulation
            std::this_thread::sleep_for(std::chrono::milliseconds(5));
        }
        
        result = sum / data.size();
        std::cout << "Analyzer " << analyzerId << " calculated risk: " 
                  << result << std::endl;
                  
    } catch (const std::exception& e) {
        std::cerr << "Analyzer " << analyzerId << " error: " 
                  << e.what() << std::endl;
        result = -1.0;
    }
}

int main(){

    constexpr int numProcessors = 5;
    constexpr int itemsPerProcessor = 20;
    sharedData.resize(numProcessors);
    std::vector<std::thread> threads;
    std::vector<std::unique_ptr<ThreadGuard>> threadGuards;
    threads.reserve(numProcessors);
    threadGuards.reserve(numProcessors);
    std::vector<double> results(numProcessors+1);

    // Create 5 threads, each processing 20 items
    for (int i = 0; i < 5; ++i) {
        threads.emplace_back(dataProcessor, i, 20);
        threadGuards.emplace_back(std::make_unique<ThreadGuard>(threads.back()));
    }
    
    // Ensure all data processors have completed before analysis.
    threadGuards.clear();
    threads.clear();

    std::cout << "Total processed: " << processedCount.load() << std::endl;
    std::cout << "Data size: " << sharedData.size() << std::endl;


    // Phase 2: Analyze each segment in parallel.
    std::vector<std::vector<double>> segments = sharedData;
    segments.emplace_back(); // Empty segment to test exceptions.
    std::vector<std::thread> analyzerThreads;
    std::vector<std::unique_ptr<ThreadGuard>> analyzerGuards;
    analyzerThreads.reserve(segments.size());
    analyzerGuards.reserve(segments.size());

    for(auto i = 0; i < segments.size(); i++){
        analyzerThreads.emplace_back(riskAnalyzer, i, std::cref(segments[i]), std::ref(results[i]));
        analyzerGuards.emplace_back(std::make_unique<ThreadGuard>(analyzerThreads.back()));
    }

    // Destroy guards: each joins its associated thread.
    analyzerGuards.clear();
    analyzerThreads.clear();


    // Phase 3: Display results.
    std::cout << "\nFinal results:\n";

    for (std::size_t i = 0; i < results.size(); ++i) {
        if (results[i] == -1.0) {
            std::cout << "Segment " << i
                    << ": analysis failed\n";
        } else {
            std::cout << "Segment " << i
                    << ": risk = " << results[i] << '\n';
        }
    }
    
    return 0;

}