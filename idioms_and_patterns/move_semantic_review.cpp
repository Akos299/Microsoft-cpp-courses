#include <cstddef>
#include <memory>
#include <stdexcept>
#include <utility>
#include <vector>
#include <iostream>
#include <chrono>
#include <algorithm>
#include <functional>
#include<cstddef>

template<typename T>
class DataBuffer {
private:
    // Add private members for buffer implementation
    std::unique_ptr<T[]> data;
    size_t bufferSize;
    size_t bufferCapacity;
    
public:
    explicit DataBuffer(size_t capacity): data(capacity ? std::make_unique<T[]>(capacity): nullptr), bufferSize(0), bufferCapacity(capacity){

    }
    
    // Implement move constructor
    DataBuffer(DataBuffer&& other) noexcept:  data(std::move(other.data)), bufferSize(std::exchange(other.bufferSize, 0)), 
    bufferCapacity(std::exchange(other.bufferCapacity, 0)) {
        // Transfer ownership efficiently
    }
    
    // Implement move assignment operator
    DataBuffer& operator=(DataBuffer&& other) noexcept {
        // Handle self-assignment and transfer ownership
        if(this != &other){
            data = std::move(other.data);
            bufferSize = std::exchange(other.bufferSize, 0);
            bufferCapacity = std::exchange(other.bufferCapacity, 0);
        }
        return *this;
    }
    
    // Disable copying to enforce move-only semantics
    DataBuffer(const DataBuffer&) = delete;
    DataBuffer& operator=(const DataBuffer&) = delete;
    
    ~DataBuffer() = default; // Smart pointers handle cleanup
    
    void addData(const T& item) {
        // Add item to buffer with bounds checking
        if(bufferSize >= bufferCapacity){
            throw std::length_error ("DataBuffer capacity exceeded");
        }
        data[bufferSize++] = item;
    }
    
    T& operator[](size_t index) {
        // Access element with bounds checking
        if(bufferSize > bufferCapacity){
            throw std::out_of_range("DataBuffer index out of range");
        }
        return data[index];
    }

    const T& operator[](std::size_t index) const {
        if (bufferSize > bufferCapacity) {
            throw std::out_of_range("DataBuffer index out of range");
        }
        return data[index];
    }

    
    size_t size() const {
        // Return current size
        return bufferSize;
    }
    
    size_t capacity() const {
        // Return buffer capacity
        return bufferCapacity;
    }
    
    void processData(const std::function<void(T&)> processor) {
        // Apply processor function to all elements
        for (size_t i = 0; i < bufferSize; i++){
            processor(data[i]);
        }
    }
};

// Performance testing function
template<typename T>
DataBuffer<T> createLargeBuffer(size_t size) {
    DataBuffer<T> buffer(size);
    for (size_t i = 0; i < size; ++i) {
        buffer.addData(static_cast<T>(i % 100));
    }
    return buffer; // Should use move semantics
}

// Explicit deep copy for benchmarking, without enabling copy semantics.
template<typename T>
DataBuffer<T> deepCopy(const DataBuffer<T>& source) {
    DataBuffer<T> copy(source.size());

    for (std::size_t i = 0; i < source.size(); ++i) {
        copy.addData(source[i]);
    }

    return copy;
}


int main(){

    constexpr std::size_t N = 1000000;
    constexpr int ITERATIONS = 100;

    // Test 1: Move construction.
    auto source = createLargeBuffer<int>(N);
    std::cout << "Before move: size = " << source.size()
              << ", capacity = " << source.capacity() << '\n';

    DataBuffer<int> destination(std::move(source));

    std::cout << "After move: source size = " << source.size()
              << ", destination size = " << destination.size() << '\n';

    
    // The moved-from object remains usable.
    // source.addData(42);
    // std::cout << "Reused source: " << source[0] << '\n';

    source = DataBuffer<int>(1);
    source.addData(42);
    std::cout << "Reused source: " << source[0] << '\n';

    // Test 2: Move assignment, including self-move protection.
    auto another = createLargeBuffer<int>(N);
    destination = std::move(another);

    destination = std::move(destination); // Safe self-move.

    std::cout << "After move assignment: size = "
              << destination.size() << '\n';


     // Test 3: Bounds checking.
    try {
        destination.addData(123); // Already at capacity.
    } catch (const std::exception& e) {
        std::cout << "Expected error: " << e.what() << '\n';
    }

    try {
        (void)destination[N]; // Out of bounds.
    } catch (const std::exception& e) {
        std::cout << "Expected error: " << e.what() << '\n';
    }

    // Test 4: Process elements.
    destination.processData([](int& value) {
        value *= 2;
    });

    std::cout << "First processed value: "
              << destination[0] << '\n';

    // Test 5: Compare repeated moves against deep copies.
    auto benchmarkSource = createLargeBuffer<int>(N);

    // Warm-up: exercise both code paths before timing.
    {
        DataBuffer<int> warmCopy = deepCopy(benchmarkSource);
        DataBuffer<int> warmMove(std::move(warmCopy));
        (void)warmMove;
    }

    // Move benchmark: transfer the same allocation back and forth.
    DataBuffer<int> movingTarget(N);
    auto moveStart = std::chrono::steady_clock::now();

    for (int i = 0; i < ITERATIONS; ++i) {
        movingTarget = std::move(benchmarkSource);
        benchmarkSource = std::move(movingTarget);
    }

    auto moveEnd = std::chrono::steady_clock::now();

    // Deep-copy benchmark: allocate and copy all elements each time.
    auto copyStart = std::chrono::steady_clock::now();

    for (int i = 0; i < ITERATIONS; ++i) {
        auto copied = deepCopy(benchmarkSource);

        // Read a value to make the result observable.
        if (copied.size() != N) {
            throw std::runtime_error("Copy verification failed");
        }
    }

    auto copyEnd = std::chrono::steady_clock::now();

    const auto moveTime =
        std::chrono::duration_cast<std::chrono::microseconds>(
            moveEnd - moveStart).count();

    const auto copyTime =
        std::chrono::duration_cast<std::chrono::microseconds>(
            copyEnd - copyStart).count();

    std::cout << "\nBenchmark (" << N << " elements, "
              << ITERATIONS << " iterations)\n";
    std::cout << "Move transfers: " << moveTime << " us\n";
    std::cout << "Deep copies:    " << copyTime << " us\n";

    if (moveTime > 0) {
        std::cout << "Copy/move time ratio: "
                  << static_cast<double>(copyTime) / moveTime
                  << "x\n";
    }

    return 0;
}