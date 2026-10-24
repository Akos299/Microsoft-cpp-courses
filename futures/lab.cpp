// ============================================================================
// Advanced Concurrency Patterns Lab - Starter Files
// Overview: Learn futures/promises, basic thread pools, and atomic operations
// Duration: 75 minutes
// Instructions: Follow the TODOs to implement advanced concurrency concepts
// ============================================================================

#include <exception>
#include <iostream>
#include <stdexcept>
#include <vector>
#include <thread>
#include <future>

#include <queue>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <functional>
#include <chrono>
#include <random>

using namespace std;

// Global atomic counter for lock-free operations
atomic<int> globalCounter{0};
atomic<bool> stopThreadPool{false};

// Simple task queue for thread pool
queue<function<void()>> taskQueue;
mutex queueMutex;
condition_variable queueCondition;

// ============================================================================
// PART 1: Futures and Promises for Asynchronous Operations
// ============================================================================

// TODO 1: Implement async calculation using promise/future
void asyncCalculation(promise<int>resultPromise, int inputValue) {
    // YOUR CODE HERE
    // 1. Simulate some complex calculation (use inputValue * inputValue + 100)
    // 2. Add a small delay using this_thread::sleep_for(chrono::milliseconds(100))
    // 3. Set the result in the promise using set_value()
    
    // Placeholder calculation
    this_thread::sleep_for(chrono::milliseconds(100));
    int result = inputValue * inputValue + 100;
    resultPromise.set_value(result);
}

// TODO 2: Create async task that might throw exception
void riskyAsyncTask(promise<string> resultPromise, int riskFactor) {
    // YOUR CODE HERE
    // 1. If riskFactor > 50, throw runtime_error("Risk too high!")
    // 2. Otherwise, set promise value to "Success with risk: " + to_string(riskFactor)
    // 3. Use promise.set_exception() to handle exceptions
    
    try {
        // YOUR IMPLEMENTATION HERE
        if(riskFactor > 50) throw runtime_error("Risk too high!");
        resultPromise.set_value("Success with risk: " + to_string(riskFactor));
    } catch (...) {
        // YOUR CODE - set exception in promise
        resultPromise.set_exception(current_exception());
    }
}

// ============================================================================
// PART 2: Basic Thread Pool Implementation
// ============================================================================

class SimpleThreadPool {
private:
    vector<thread> workers;
    int numThreads;

public:
    SimpleThreadPool(int threads) : numThreads(threads) {
        initializePool();
    }
    
    ~SimpleThreadPool() {
        shutdown();
    }
    
    // TODO 3: Initialize the thread pool
    void initializePool() {
        // YOUR CODE HERE
        // 1. Create numThreads worker threads
        // 2. Each worker should run the workerFunction
        // 3. Store threads in the workers vector
       
        for (int i = 0; i < numThreads; ++i) {
            workers.emplace_back(&SimpleThreadPool::workerFunction, this);
        }
         cout << "Thread pool initialized with " << numThreads << " worker threads" << endl;
    }
    
    // TODO 4: Implement worker function that processes tasks from queue
    void workerFunction() {
        // YOUR CODE HERE
        // 1. Loop while !stopThreadPool.load()
        // 2. Wait for tasks using condition variable
        // 3. Pop task from queue and execute it
        // 4. Use proper synchronization (unique_lock, condition_variable)
        
        while (!stopThreadPool.load()) {
            function<void()> task;
        {
            unique_lock<mutex> lock(queueMutex);
            queueCondition.wait(lock, []{return !taskQueue.empty() || stopThreadPool.load();});
            // Check if we should stop and queue is empty
            if (stopThreadPool.load() && taskQueue.empty()) {
                break;
            }
            // Get task from queue
            if (!taskQueue.empty()) {
                task = std::move(taskQueue.front());
                taskQueue.pop();
            }
       
        }
        // Execute task outside of lock
            if (task) {
                task();
            }
        }
    }
    
    // TODO 5: Add task to the thread pool
    void enqueue(function<void()> task) {
        // YOUR CODE HERE
        // 1. Lock the queue mutex
        // 2. Add task to the queue
        // 3. Notify one waiting worker thread
        {
            lock_guard<mutex> lock(queueMutex);
            taskQueue.push(std::move(task));
        }
        queueCondition.notify_one();
    }
    
    // TODO 6: Shutdown the thread pool
    void shutdown() {
        // YOUR CODE HERE
        // 1. Set stopThreadPool to true
        // 2. Notify all waiting threads
        // 3. Join all worker threads
        
     
        // Signal shutdown to all threads
        stopThreadPool.store(true);
        
        // Wake up all waiting threads
        queueCondition.notify_all();
        
        // Wait for all threads to complete
        for (auto& worker : workers) {
            if (worker.joinable()) {
                worker.join();
            }
        }
        
        workers.clear();
        cout << "Thread pool shutdown completed" << endl;
        

    }
};

// ============================================================================
// PART 3: Atomic Operations and Lock-Free Programming
// ============================================================================

// Implement lock-free increment function
void atomicIncrement(int iterations) { 
    for (int i = 0; i < iterations; ++i) {
        // YOUR CODE - atomic increment
        globalCounter.fetch_add(1);
    }
}

// Implement compare-and-swap operation
bool atomicCompareAndSwap(atomic<int>& target, int expected, int desired) {
    return target.compare_exchange_strong(expected,desired);
}

// ============================================================================
// PART 4: Combining Patterns - Async Task Processing
// ============================================================================

// Process multiple tasks asynchronously and collect results
vector<future<int>> processTasksBatch(SimpleThreadPool& pool, const vector<int>& inputData) {
    vector<future<int>> futures;
     futures.reserve(inputData.size());
    
    // YOUR CODE HERE
    // 1. For each input value, create a promise/future pair
    // 2. Submit a task to the thread pool that processes the value and sets the promise
    // 3. Store the future in the futures vector
    // 4. Return the vector of futures
    
    for (int value : inputData) {
        // YOUR CODE - create promise/future and submit task
        auto promise_ = std::make_shared<promise<int>>();
        future<int> future_ = promise_->get_future();

        pool.enqueue([promise_, value](){
            // Simulate some processing
            this_thread::sleep_for(chrono::milliseconds(50));
            
            // Calculate result (square the input)
            int result = value * value;
            
            // Set the promise value
            promise_->set_value(result);
        });
        futures.push_back(std::move(future_));
     
    }
    
    return futures;
}

// Test functions for validation
void testFuturesAndPromises() {
    cout << "\n=== Testing Futures and Promises ===" << endl;
    
    // Test basic promise/future
    promise<int> calcPromise;
    future<int> calcFuture = calcPromise.get_future();
    
    thread calcThread(asyncCalculation, std::move(calcPromise), 10);
    
    cout << "Waiting for async calculation..." << endl;
    int result = calcFuture.get();
    cout << "Calculation result: " << result << endl;
    
    calcThread.join();
    
    // Test exception handling
    promise<string> riskPromise;
    future<string> riskFuture = riskPromise.get_future();
    
    thread riskThread(riskyAsyncTask, std::move(riskPromise), 25);
    
    try {
        string riskResult = riskFuture.get();
        cout << "Risk task result: " << riskResult << endl;
    } catch (const exception& e) {
        cout << "Risk task failed: " << e.what() << endl;
    }
    
    riskThread.join();
}

void testThreadPool() {
    cout << "\n=== Testing Thread Pool ===" << endl;
    
    SimpleThreadPool pool(4);
    
    // Submit several tasks
    for (int i = 1; i <= 8; ++i) {
        pool.enqueue([i]() {
            cout << "Task " << i << " executing on thread " 
                 << this_thread::get_id() << endl;
            this_thread::sleep_for(chrono::milliseconds(200));
            cout << "Task " << i << " completed" << endl;
        });
    }
    
    // Let tasks complete
    this_thread::sleep_for(chrono::seconds(2));
    
    cout << "Thread pool test completed" << endl;
}

void testAtomicOperations() {
    cout << "\n=== Testing Atomic Operations ===" << endl;
    
    globalCounter.store(0);
    
    // Test atomic increment with multiple threads
    vector<thread> incrementThreads;
    for (int i = 0; i < 4; ++i) {
        incrementThreads.emplace_back(atomicIncrement, 1000);
    }
    
    for (auto& t : incrementThreads) {
        t.join();
    }
    
    cout << "Expected counter value: 4000" << endl;
    cout << "Actual counter value: " << globalCounter.load() << endl;
    cout << "Atomic operations " << (globalCounter.load() == 4000 ? "PASSED" : "FAILED") << endl;
    
    // Test compare and swap
    atomic<int> testValue{10};
    bool swapped = atomicCompareAndSwap(testValue, 10, 20);
    cout << "Compare and swap (10->20): " << (swapped ? "SUCCESS" : "FAILED") 
         << ", value is now: " << testValue.load() << endl;
    
    bool notSwapped = atomicCompareAndSwap(testValue, 10, 30);
    cout << "Compare and swap (10->30): " << (notSwapped ? "SUCCESS" : "FAILED") 
         << ", value is still: " << testValue.load() << endl;
}

void testCombinedPatterns() {
    cout << "\n=== Testing Combined Patterns ===" << endl;
    
    SimpleThreadPool pool(3);
    vector<int> inputData = {1, 2, 3, 4, 5, 6, 7, 8};
    
    cout << "Processing batch of " << inputData.size() << " tasks..." << endl;
    
    auto futures = processTasksBatch(pool, inputData);
    
    cout << "Collecting results..." << endl;
    for (size_t i = 0; i < futures.size(); ++i) {
        int result = futures[i].get();
        cout << "Task " << i + 1 << " result: " << result << endl;
    }
    
    cout << "Combined patterns test completed" << endl;
}

int main() {
    cout << "=== Advanced Concurrency Patterns Lab ===" << endl;
    cout << "Learning futures/promises, thread pools, and atomic operations" << endl;
    
    testFuturesAndPromises();
    testThreadPool();
    testAtomicOperations();
    testCombinedPatterns();
    
    cout << "\n=== Lab Complete! ===" << endl;
    cout << "You've successfully implemented advanced concurrency patterns!" << endl;
    
    return 0;
}

/*
REQUIREMENTS CHECKLIST:
□ Implement async calculation using promise/future with proper result setting
□ Handle exceptions in async tasks using promise.set_exception()
□ Create basic thread pool with worker threads and task queue
□ Implement thread-safe task enqueuing with condition variable notification
□ Use atomic operations for lock-free counter increments
□ Implement compare-and-swap using atomic compare_exchange_strong
□ Combine patterns: async task processing with thread pool and futures
□ Demonstrate proper cleanup and resource management

COMMON MISTAKES TO AVOID:
- Not setting promise values or handling exceptions properly
- Forgetting to notify condition variables when enqueuing tasks
- Using non-atomic operations on shared atomic variables
- Not joining threads properly in thread pool shutdown
- Race conditions between task submission and worker thread processing

VALIDATION:
- Async calculations should return correct results through futures
- Thread pool should execute all submitted tasks
- Atomic counter should reach exactly 4000 without race conditions
- Compare-and-swap should work correctly with expected/actual values
- Combined pattern should process all input values and return results
*/