// ============================================================================
// Basic Synchronization Mechanisms Lab - Starter Files
// Overview: Learn mutex types, condition variables, and deadlock prevention
// Duration: 60 minutes
// Instructions: Follow the TODOs to implement fundamental synchronization concepts
// ============================================================================

#include <iostream>
#include <vector>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <chrono>
#include <queue>
#include <atomic>
#include <functional>


using namespace std;

// Shared resources that need synchronization
queue<int> sharedQueue;
mutex queueMutex;
condition_variable queueCondition;
atomic<bool> stopProcessing{false};

// Thread-safe counter class demonstration
class ThreadSafeCounter {
private:
    int counter_;
    mutable mutex mtx_;  // mutable allows locking in const methods

public:
    ThreadSafeCounter() : counter_(0) {}
    
    // Implement thread-safe increment
    void increment() {
        // Use lock_guard to protect the counter increment
        lock_guard<mutex> lock(mtx_);
        counter_++;
    }
    
    // Implement thread-safe getValue
    int getValue() const { 
        // Use lock_guard to protect counter access
        // Return the current counter value
        lock_guard<mutex> lock(mtx_);
        return counter_;
    }
};

// Deadlock demonstration with two mutexes
mutex mutexA;
mutex mutexB;

void functionA() {
    // Implement proper lock ordering to prevent deadlock
    // This function should acquire mutexA first, then mutexB
    lock_guard<mutex> lockA(mutexA);
    lock_guard<mutex> lockB(mutexB);
    
    cout << "Function A acquired both locks" << endl;
    this_thread::sleep_for(chrono::milliseconds(100));
    
    // Locks will be automatically released by lock_guard destructors
}

void functionB() {
    // Implement same lock ordering as functionA to prevent deadlock
    // This function should also acquire mutexA first, then mutexB (same order!)
    lock_guard<mutex> lockA(mutexA);
    lock_guard<mutex> lockB(mutexB);
    cout << "Function B acquired both locks" << endl;
    this_thread::sleep_for(chrono::milliseconds(100));
    
    // Locks will be automatically released by lock_guard destructors
}

// Producer function for producer-consumer pattern
void producer(int producerId) {
    for (int i = 1; i <= 5; ++i) {
        // Implement thread-safe queue insertion
        // 1. Lock the queue mutex
        // 2. Add item (producerId * 100 + i) to the queue  
        // 3. Notify waiting consumers
        {
            lock_guard<mutex> lock_(queueMutex);
            sharedQueue.push(producerId*100 + i);
        }
        queueCondition.notify_one();
        
        cout << "Producer " << producerId << " added item: " << (producerId * 100 + i) << endl;
        this_thread::sleep_for(chrono::milliseconds(200));
    }
}

// Consumer function for producer-consumer pattern
void consumer(int consumerId) {
    while (!stopProcessing.load()) {
        // Implement thread-safe queue removal with condition variable
        // 1. Create unique_lock for the mutex
        // 2. Wait on condition variable until queue has items or stop is requested
        // 3. If stop requested and queue empty, break
        // 4. Remove and process item from queue
        
        // YOUR CODE HERE
        unique_lock<mutex> unq_lock(queueMutex);
        queueCondition.wait(unq_lock, []{return !sharedQueue.empty() || stopProcessing.load();});
        if(stopProcessing.load() && sharedQueue.empty())
            break;
            
        if(!sharedQueue.empty()){
             int item = sharedQueue.front();  // Replace with item from queue
            sharedQueue.pop();
            unq_lock.unlock();
            cout << "Consumer " << consumerId << " processed item: " << item << endl;
            this_thread::sleep_for(chrono::milliseconds(300));
        }
    }
}

// Demonstration of different mutex types
void demonstrateMutexTypes() {
    cout << "\n=== Mutex Types Demonstration ===" << endl;
    
    // Demonstrate recursive_mutex
    // Create a recursive_mutex and show it can be locked multiple times by same thread
    recursive_mutex rec_mutex;
    
    // Function that locks recursively
    function<void(int)> recursiveLock = [&](int depth) {
        if (depth <= 0) return;
        
        // TODO: Lock the recursive mutex here
        lock_guard<recursive_mutex> rec_lock(rec_mutex);
        cout << "Recursive lock depth: " << depth << endl;
        
        recursiveLock(depth - 1);  // Recursive call with same mutex locked
        
        // TODO: Unlock happens automatically with lock_guard
    };
    
    thread recursiveThread([&]() {
        recursiveLock(3);  // This should work with recursive_mutex
    });
    
    recursiveThread.join();
}

int main() {
    cout << "=== Basic Synchronization Mechanisms Lab ===" << endl;
    cout << "Learning mutex types, condition variables, and deadlock prevention" << endl;
    
    // Test 1: Thread-Safe Counter
    cout << "\n--- Test 1: Thread-Safe Counter ---" << endl;
    ThreadSafeCounter counter;
    vector<thread> counterThreads;
    
    // Create threads that increment the counter
    for (int i = 0; i < 5; ++i) {
        counterThreads.emplace_back([&counter]() {
            for (int j = 0; j < 10; ++j) {
                counter.increment();
                this_thread::sleep_for(chrono::milliseconds(10));
            }
        });
    }
    
    // Wait for all threads to complete
    for (auto& t : counterThreads) {
        t.join();
    }
    
    cout << "Final counter value: " << counter.getValue() << " (should be 50)" << endl;
    
    // Test 2: Deadlock Prevention
    cout << "\n--- Test 2: Deadlock Prevention ---" << endl;
    cout << "Testing proper lock ordering..." << endl;
    
    thread threadA(functionA);
    thread threadB(functionB);
    
    threadA.join();
    threadB.join();
    cout << "No deadlock occurred - proper lock ordering implemented!" << endl;
    
    // Test 3: Producer-Consumer Pattern
    cout << "\n--- Test 3: Producer-Consumer Pattern ---" << endl;
    
    vector<thread> producers;
    vector<thread> consumers;
    
    // Create producer threads
    for (int i = 1; i <= 2; ++i) {
        producers.emplace_back(producer, i);
    }
    
    // Create consumer threads
    for (int i = 1; i <= 2; ++i) {
        consumers.emplace_back(consumer, i);
    }
    
    // Wait for producers to finish
    for (auto& t : producers) {
        t.join();
    }
    
    // Give consumers time to process remaining items
    this_thread::sleep_for(chrono::seconds(2));
    
    // Stop consumers
    stopProcessing.store(true);
    queueCondition.notify_all();
    
    for (auto& t : consumers) {
        t.join();
    }
    
    // Test 4: Mutex Types
    demonstrateMutexTypes();
    
    cout << "\n=== Lab Complete! ===" << endl;
    cout << "You've successfully implemented basic synchronization mechanisms!" << endl;
    
    return 0;
}

/*
REQUIREMENTS CHECKLIST:
□ Implement thread-safe counter using mutex and lock_guard
□ Create proper lock ordering to prevent deadlocks
□ Use condition variables for producer-consumer coordination
□ Demonstrate different mutex types (regular and recursive)
□ Handle thread synchronization without race conditions
□ Apply RAII principles with lock_guard for automatic cleanup

COMMON MISTAKES TO AVOID:
- Inconsistent lock ordering leading to deadlocks
- Forgetting to notify condition variables after adding items
- Using wrong mutex types for the scenario
- Not checking conditions properly in condition_variable::wait
- Race conditions when accessing shared data

VALIDATION:
- Counter should reach exactly 50 without race conditions
- No deadlocks should occur in the two-mutex scenario
- Producer-consumer should work without lost or corrupted items
- Recursive mutex should allow multiple locks by same thread
*/
