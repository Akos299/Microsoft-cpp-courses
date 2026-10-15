#include <iostream>
#include <vector>
#include <list>
#include <deque>
#include <chrono>
#include <random>
#include <iomanip>
#include <algorithm>

class ContainerBenchmark {
private:
static const int DATA_SIZE = 50000;

// How does a Mersenne Twister Engine work?
    std::mt19937 rng{std::random_device{}()};
    std::uniform_int_distribution<int> dist{1, 100000};
// Helper function to measure execution time

    template<typename Func>
    double measureTime(Func&& func) {
        // start the chrono
        auto start = std::chrono::high_resolution_clock::now();
        func();
        auto end = std::chrono::high_resolution_clock::now();
        // stop the chrono
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
        return duration.count() / 1000.0; // Convert to milliseconds
    }

public:
void benchmarkSequenceContainers() {
            std::cout << "=== SEQUENCE CONTAINER PERFORMANCE COMPARISON ===" << std::endl;
            std::cout << "Data size: " << DATA_SIZE << " elements\n" << std::endl;

            // Test containers
            std::vector<int> testVector;
            std::list<int> testList;
            std::deque<int> testDeque;

            // Reserve space for vector to avoid reallocation during benchmark
            testVector.reserve(DATA_SIZE);

            // Use fixed decimal notation rather than scientific notation. And with 3 decimals.
            std::cout << std::fixed << std::setprecision(3);
            std::cout << std::setw(20) << "Operation"
                << std::setw(15) << "Vector (ms)"
                << std::setw(15) << "List (ms)"
                << std::setw(15) << "Deque (ms)" << std::endl;
                std::cout << std::string(65, '-') << std::endl;

            // Test back insertion performance
            double vectorBackTime = measureTime([&]() {
                for (int i = 0; i < DATA_SIZE; ++i) {
                    testVector.push_back(dist(rng));}
            });

            double listBackTime = measureTime([&]() {
            for (int i = 0; i < DATA_SIZE; ++i) {
            testList.push_back(dist(rng));
            }});

            double dequeBackTime = measureTime([&]() {
            for (int i = 0; i < DATA_SIZE; ++i) {
            testDeque.push_back(dist(rng));
            }});


        std::cout << std::setw(20) << "Back Insertion" 
        << std::setw(15) << vectorBackTime
        << std::setw(15) << listBackTime 
        << std::setw(15) << dequeBackTime << std::endl;

        // Test front insertion performance (skip vector due to poor performance)
        std::list<int> frontTestList;
        std::deque<int> frontTestDeque;

        double listFrontTime = measureTime([&]() {
        for (int i = 0; i < DATA_SIZE; ++i) {
        frontTestList.push_front(dist(rng));}
        });

        double dequeFrontTime = measureTime([&]() {
        for (int i = 0; i < DATA_SIZE; ++i) {
        frontTestDeque.push_front(dist(rng));}
        });

        std::cout << std::setw(20) << "Front Insertion"
        << std::setw(15) << "N/A (O(n))"
        << std::setw(15) << listFrontTime
        << std::setw(15) << dequeFrontTime << std::endl;

        // Test random access performance. The random access because "pos" is a random integer between 1 and 9999.
        std::vector<int> positions;
        for (int i = 0; i < 10000; ++i) {
        positions.push_back(std::uniform_int_distribution<int>{0, static_cast<int>(testVector.size() - 1)}(rng));
        }

        double vectorAccessTime = measureTime([&]() {
        volatile int sum = 0; // Prevent optimization
        for (int pos : positions) {
        sum += testVector[pos];
        }
        });

        double dequeAccessTime = measureTime([&]() {
        volatile int sum = 0;
        for (int pos : positions) {
        sum += testDeque[pos];
        }
        });

        // List random access is O(n) - demonstrate with smaller sample
        std::vector<int> smallPositions(positions.begin(), positions.begin() + 1000);
        double listAccessTime = measureTime([&]() {
        volatile int sum = 0;
        for (int pos : smallPositions) {
        auto it = testList.begin();
        std::advance(it, pos);
        sum += *it;
        }
        }) * 10; 

        // Scale up to compare with others
        std::cout << std::setw(20) << "Random Access"
        << std::setw(15) << vectorAccessTime
        << std::setw(15) << listAccessTime << " (est)"
        << std::setw(15) << dequeAccessTime << std::endl;
        std::cout << std::string(65, '-') << std::endl;
        std::cout << "Performance Analysis:" << std::endl;
        std::cout << " Vector: Excellent random access, good back insertion, poor front insertion" << std::endl;
        std::cout << " List: Good insertion anywhere, poor random access" << std::endl;
        std::cout << " Deque: Good insertion at ends, good random access" << std::endl;
}

void benchmarkMiddleOperations() {
        std::cout << "\n=== MIDDLE INSERTION/DELETION COMPARISON ===" << std::endl;
        const int SMALLER_SIZE = 10000; // Reduced for middle operations
        // Prepare containers with initial data
        std::vector<int> testVector;
        std::list<int> testList;
        std::deque<int> testDeque;

        for (int i = 0; i < SMALLER_SIZE; ++i) {
        int value = dist(rng);
        testVector.push_back(value);
        testList.push_back(value);
        testDeque.push_back(value);}

        std::cout << std::fixed << std::setprecision(3);
        std::cout << std::setw(20) << "Operation"
        << std::setw(15) << "Vector (ms)"
        << std::setw(15) << "List (ms)"
        << std::setw(15) << "Deque (ms)" << std::endl;
        std::cout << std::string(65, '-') << std::endl;


        // Test middle insertion
        double vectorMiddleInsert = measureTime([&]() {
        for (int i = 0; i < 1000; ++i) {
        auto it = testVector.begin() + testVector.size() / 2;
        // auto it = testVector.begin();
        std::advance(it, testList.size() / 2);
        testVector.insert(it, dist(rng));
        }});


        double listMiddleInsert = measureTime([&]() {
        for (int i = 0; i < 1000; ++i) {
        auto it = testList.begin();
        std::advance(it, testList.size() / 2);
        testList.insert(it, dist(rng));
        }});


        double dequeMiddleInsert = measureTime([&]() {
        for (int i = 0; i < 1000; ++i) {
        auto it = testDeque.begin() + testDeque.size() / 2;
        testDeque.insert(it, dist(rng));}});


        std::cout << std::setw(20) << "Middle Insertion"
        << std::setw(15) << vectorMiddleInsert
        << std::setw(15) << listMiddleInsert
        << std::setw(15) << dequeMiddleInsert << std::endl;
        std::cout << std::string(65, '-') << std::endl;
        std::cout << "Middle Operations Analysis:" << std::endl;
        std::cout << " Vector: O(n) - Must shift all subsequent elements" << std::endl;
        std::cout << " List: O(1) - Direct insertion at position" << std::endl;
        std::cout << " Deque: O(n) - Must shift elements, but better than vector" << std::endl;
}

};




int main() {
    std::cout << "STL SEQUENCE CONTAINER PERFORMANCE ANALYSIS" << std::endl;
    std::cout << "===========================================" << std::endl;
    ContainerBenchmark benchmark;
    benchmark.benchmarkSequenceContainers();
    benchmark.benchmarkMiddleOperations();
    return 0;
}