#include <condition_variable>
#include <cstddef>
#include <exception>
#include <functional>
#include <iostream>
#include <mutex>
#include <queue>
#include <stdexcept>
#include <string>
#include <thread>
#include <vector>


class ThreadSafeQueue {
private:
    mutable std::mutex mutex_;
    std::queue<double> queue_;
    std::condition_variable condition_;
    bool finished_ = false;
    
public:
    void push(double item) {
        {
            std::lock_guard<std::mutex> lock(mutex_);

            if (finished_) {
                throw std::logic_error("Queue is closed");
            }

            queue_.push(item);
        }

        condition_.notify_one();
    }
        
    bool pop(double& item) {
        std::unique_lock<std::mutex> lock(mutex_);
        condition_.wait(lock, [this] { return !queue_.empty() || finished_; });
        
        if (queue_.empty()) {
            return false;  // No more items and producer finished
        }
        
        item = queue_.front();
        queue_.pop();
        return true;
    }
    
    void finish() {
        {
            std::lock_guard<std::mutex> lock(mutex_);
            finished_ = true;
        }
        condition_.notify_all();
    }
    
    size_t size() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return queue_.size();
    }
};

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

void produce(ThreadSafeQueue& queue,
             int producer_id,
             int count) {
    for (int i = 0; i < count; ++i) {
        queue.push(producer_id * 1000.0 + i);
    }
}

void consume(ThreadSafeQueue& queue) {
    double item;

    while (queue.pop(item)) {
        // Replace with the actual processing operation.
        const double result = item * item;
        (void)result;
    }
}


int main(int argc, char* argv[]){
    try {
        if(argc != 4){
            throw std::runtime_error("expected 3 arguments: ./prog  nb_producer nb_consumer nb_data_per_prod");
        }

        const int num_producers = std::stoi(argv[1]);
        const int num_consumers = std::stoi(argv[2]);
        const int items_per_producer = std::stoi(argv[3]);

        if (num_producers <= 0 ||
            num_consumers <= 0 ||
            items_per_producer < 0) {
            throw std::invalid_argument("Invalid argument values");
        }


        ThreadSafeQueue queue;
        std::vector<std::thread> producers;
        std::vector<std::unique_ptr<ThreadGuard>> producersGuard;
        std::vector<std::thread> consumers;
        std::vector<std::unique_ptr<ThreadGuard>> consumersGuard;

        producers.reserve(num_producers);
        consumers.reserve(num_consumers);
        producersGuard.reserve(num_producers);
        consumersGuard.reserve(num_consumers);

        for (int i = 0; i < num_consumers; ++i) {
            consumers.emplace_back(consume, std::ref(queue));
            consumersGuard.emplace_back(std::make_unique<ThreadGuard>(consumers.back()));
        }

        for(int i = 0; i < num_producers; ++i){
            producers.emplace_back(produce, std::ref(queue), i, items_per_producer);
            producersGuard.emplace_back(std::make_unique<ThreadGuard>(producers.back()));
        }

        // 1. Wait for all producers.
        producersGuard.clear();

        // 2. Wake consumers and signal end of production.
        queue.finish();

        // 3. Wait for all consumers.
        consumersGuard.clear();

        // 4. The std::thread objects are now no longer joinable.
        producers.clear();
        consumers.clear();

        std::cout << "Processing completed.\n";


    } catch (const std::exception& e) {
    std::cerr << " Error" 
                  << e.what() << std::endl;
        
    }

    return 0;
}