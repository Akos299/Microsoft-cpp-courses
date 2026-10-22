// ============================================================================
// Basic Multithreading Lab - Starter Files
// Overview: Learn fundamental thread creation, lifecycle management, and basic synchronization
// Duration: 45 minutes
// Instructions: Follow the TODOs to implement basic multithreading concepts
// ============================================================================

#include <iostream>
#include <string>
#include <vector>
#include <thread>
#include <mutex>
#include <atomic>
#include <chrono>
#include <numeric>

using namespace std;

// Global variables for demonstration
mutex printMutex;  // For thread-safe printing
atomic<int> taskCounter{0};  // Atomic counter for tasks

/**
 * Simple calculation function that each thread will execute
 * Calculates sum of numbers from start to end
 */
int calculateSum(int start, int end) {
    int sum = 0;
    for (int i = start; i <= end; i++) {
        sum += i;
    }
    
    // Simulate some processing time
    this_thread::sleep_for(chrono::milliseconds(100));
    
    return sum;
}

/**
 * Thread-safe printing function
 * Uses mutex to ensure clean output when multiple threads print
 */
void safePrint(const string& message) {
    // Implement thread-safe printing using printMutex
    // Use lock_guard to automatically manage the mutex
    // Print the message safely

    lock_guard<std::mutex> lock(printMutex);
    cout << message <<  endl;
}

/**
 * Worker function that each thread will execute
 * Demonstrates basic thread operations and synchronization
 */
void workerFunction(int threadId, int start, int end) {
    // Implement the worker function
    // 1. Print a message that this thread is starting (use safePrint)
    // 2. Calculate the sum using calculateSum function
    // 3. Increment the taskCounter atomically  
    // 4. Print the result (use safePrint)
    const string workerMsg = to_string(threadId) + " is starting \n";
    safePrint(workerMsg);
    int partialSum = calculateSum(start, end);
    taskCounter.fetch_add(1);
    const string workerSumMsg = to_string(threadId) + " has partial sum of " + to_string(partialSum) + "\n";
    safePrint(workerSumMsg);
}

int main() {
    cout << "=== Basic Multithreading Lab ===" << endl;
    cout << "Learning thread creation, lifecycle management, and synchronization" << endl;
    
    // TODO 3: Create and manage threads
    const int NUM_THREADS = 4;
    const int RANGE_PER_THREAD = 250;  // Each thread processes 250 numbers
    
    // Create vector to store thread objects
    // YOUR CODE HERE - declare vector<thread> threads
    vector<thread> ths;
    ths.reserve(NUM_THREADS);
    
    cout << "\nStarting " << NUM_THREADS << " worker threads..." << endl;
    
    // TODO 4: Create and launch threads
    // Create NUM_THREADS threads, each running workerFunction
    // Thread i should process numbers from (i*RANGE_PER_THREAD + 1) to ((i+1)*RANGE_PER_THREAD)
    // Example: Thread 0 processes 1-250, Thread 1 processes 251-500, etc.
    
    // YOUR CODE HERE - create threads using emplace_back or push_back

    for(auto idx = 0; idx < NUM_THREADS; idx++){
        int start = idx*RANGE_PER_THREAD + 1;
        int end = (idx+1)*RANGE_PER_THREAD;
        ths.emplace_back(workerFunction, idx, start, end );
    }
    
    cout << "All threads started. Waiting for completion..." << endl;
    
    // Wait for all threads to complete
    // Use join() on each thread to wait for completion
    for (auto& t : ths) {
        if (t.joinable()) {
            t.join();
        }
    }
    
    cout << "\nAll threads completed!" << endl;
    cout << "Total tasks completed: " << taskCounter.load() << endl;
    
    // Demonstrate thread detaching (BONUS)
    // Create one more thread that runs independently
    // Use detach() instead of join()
    // Note: This thread may or may not complete before main() exits
    
    cout << "\nStarting detached thread..." << endl;

    // YOUR CODE HERE - create and detach a thread
    thread detachedThread([]() {
        safePrint("Detached thread running independently!");
        this_thread::sleep_for(chrono::milliseconds(150));
        safePrint("Detached thread finished!");
    });
    
    // Detach the thread to run independently
    detachedThread.detach();
    
    // Give detached thread a chance to run
    this_thread::sleep_for(chrono::milliseconds(200));
    
    cout << "Main thread finishing. Detached thread may still be running." << endl;
    
    return 0;
}

/*
REQUIREMENTS CHECKLIST:
□ Implement thread-safe printing with mutex and lock_guard
□ Create worker function that processes data and updates atomic counter
□ Create multiple threads using std::thread constructor
□ Use join() to wait for thread completion
□ Demonstrate proper thread lifecycle management
□ (BONUS) Create and detach a thread to show difference from join()

COMMON MISTAKES TO AVOID:
- Forgetting to join threads before main() exits
- Not using mutex for shared resource access (printing)
- Incorrect atomic variable usage
- Thread resource leaks from not joining or detaching properly

VALIDATION:
- Program should create 4 threads that each calculate a sum
- Output should be thread-safe (no garbled text)
- All threads should complete before main() exits
- Atomic counter should show correct number of completed tasks
*/