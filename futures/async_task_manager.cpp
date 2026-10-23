#include <iostream>
#include <future>
#include <vector>
#include <chrono>
#include <random>
#include <functional>
#include <exception>
#include <memory>
#include <thread>

template<typename T>
class AsyncTaskManager {
private:
    std::vector<std::future<T>> activeTasks_;
    mutable std::mutex tasksMutex_;
    
public:
    // Submit async task with custom executor
    template<typename Func, typename... Args>
    void submitTask(Func&& func, Args&&... args){
        auto task = std::make_shared<std::packaged_task<T()>>(
            std::bind(std::forward<Func>(func), std::forward<Args>(args)...));
        
        std::future<T> future = task->get_future();
        
        // Execute in separate thread
        std::thread([task]() {
            try {
                (*task)();
            } catch (...) {
                // Exception automatically captured by packaged_task
            }
        }).detach();
        
        {
            std::lock_guard<std::mutex> lock(tasksMutex_);
            activeTasks_.push_back(std::move(future));
        }
        
        
    }
    
    // Wait for all tasks with timeout
    std::vector<T> waitForAll(std::chrono::milliseconds timeout = std::chrono::milliseconds(5000)) {
        std::vector<T> results;
        
        std::lock_guard<std::mutex> lock(tasksMutex_);
        for (auto& future : activeTasks_) {
            try {
                if (future.wait_for(timeout) == std::future_status::ready) {
                    results.push_back(future.get());
                } else {
                    std::cout << "Task timed out" << std::endl;
                }
            } catch (const std::exception& e) {
                std::cout << "Task failed: " << e.what() << std::endl;
            }
        }
        
        activeTasks_.clear();
        return results;
    }
    
    // Get completion status of all tasks
    struct TaskStatus {
        size_t completed = 0;
        size_t pending = 0;
        size_t failed = 0;
    };
    
    TaskStatus getStatus() const {
        TaskStatus status;
        
        std::lock_guard<std::mutex> lock(tasksMutex_);
        for (const auto& future : activeTasks_) {
            auto taskStatus = future.wait_for(std::chrono::milliseconds(0));
            
            if (taskStatus == std::future_status::ready) {
                status.completed++;
            } else {
                status.pending++;
            }
        }
        
        return status;
    }
};

// Complex computation that might fail
struct DataAnalysis {
    int datasetId;
    double result;
    std::chrono::milliseconds processingTime;
    
    static DataAnalysis processDataset(int id, int complexity) {
        std::random_device rd;
        std::mt19937 gen(rd());
        
        // Simulate variable processing time
        std::uniform_int_distribution<> timeDist(100, 1000);
        auto processingTime = std::chrono::milliseconds(timeDist(gen));
        
        std::this_thread::sleep_for(processingTime);
        
        // Simulate potential failures
        std::uniform_int_distribution<> failDist(1, 100);
        if (failDist(gen) <= 10) { // 10% failure rate
            throw std::runtime_error("Dataset processing failed for ID " + std::to_string(id));
        }
        
        // Complex mathematical computation
        double result = 0.0;
        for (int i = 0; i < complexity * 1000; ++i) {
            result += std::sin(i) * std::cos(i * id);
        }
        
        return DataAnalysis{id, result, processingTime};
    }
};

int main() {
    AsyncTaskManager<DataAnalysis> manager;
    
    std::cout << "Starting distributed analytics simulation..." << std::endl;
    
    // Submit analysis tasks with varying complexity
    std::vector<std::future<DataAnalysis>> futures;
    for (int i = 1; i <= 20; ++i) {
        int complexity = 10 + (i % 5) * 5; // Varying complexity levels
       
        manager.submitTask(DataAnalysis::processDataset, i, complexity);
        // futures.push_back(std::move(future));
    }
    
    // Monitor progress
    std::thread monitoring([&manager]() {
        for (int i = 0; i < 20; ++i) {
            std::this_thread::sleep_for(std::chrono::milliseconds(500));
            auto status = manager.getStatus();
            std::cout << "Status - Completed: " << status.completed 
                      << ", Pending: " << status.pending << std::endl;
        }
    });
    
    // Collect results
    auto results = manager.waitForAll();
    monitoring.join();
    
    // Aggregate successful results
    double totalResult = 0.0;
    std::chrono::milliseconds totalTime{0};
    
    for (const auto& analysis : results) {
        totalResult += analysis.result;
        totalTime += analysis.processingTime;
    }
    
    std::cout << "Analysis complete - " << results.size() << " successful datasets" << std::endl;
    std::cout << "Total computation result: " << totalResult << std::endl;
    std::cout << "Total processing time: " << totalTime.count() << " ms" << std::endl;
    
    return 0;
}