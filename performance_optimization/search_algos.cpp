
#include <algorithm>
#include <chrono>
#include <cstddef>
#include <iostream>
#include <limits>
#include <random>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>

class SearchOptimization {
public:
    static int linearSearch(
        const std::vector<int>& data,
        int target)
    {
        for (std::size_t i = 0; i < data.size(); ++i) {
            if (data[i] == target)
                return static_cast<int>(i);
        }

        return -1;
    }

    // Requires sorted data.
    static int binarySearch(
        const std::vector<int>& sortedData,
        int target)
    {
        std::size_t left = 0;
        std::size_t right = sortedData.size();

        // Half-open interval [left, right).
        while (left < right) {
            const std::size_t mid =
                left + (right - left) / 2;

            if (sortedData[mid] == target)
                return static_cast<int>(mid);

            if (sortedData[mid] < target)
                left = mid + 1;
            else
                right = mid;
        }

        return -1;
    }

    // Maps each value to its first index.
    static void buildHashMap(
        const std::vector<int>& data,
        std::unordered_map<int, int>& hashMap)
    {
        hashMap.clear();
        hashMap.reserve(data.size());

        for (std::size_t i = 0; i < data.size(); ++i) {
            hashMap.try_emplace(
                data[i],
                static_cast<int>(i)
            );
        }
    }

    static int hashSearch(
        const std::unordered_map<int, int>& hashMap,
        int target)
    {
        const auto it = hashMap.find(target);

        return it != hashMap.end() ? it->second : -1;
    }
};

std::vector<int> generateTestData(int size) {
    std::vector<int> data(static_cast<std::size_t>(size));

    std::mt19937 gen(std::random_device{}());
    std::uniform_int_distribution<int> dist(1, size * 10);

    for (int& value : data)
        value = dist(gen);

    return data;
}

template <typename Function>
double benchmarkMs(Function&& function, int repetitions) {
    // Warm-up.
    volatile int result = function();
    (void)result;

    double totalMs = 0.0;

    for (int i = 0; i < repetitions; ++i) {
        const auto start = std::chrono::steady_clock::now();

        result = function();

        const auto end = std::chrono::steady_clock::now();

        totalMs += std::chrono::duration<double, std::milli>(
            end - start
        ).count();
    }

    // Ensure the result remains observable.
    (void)result;

    return totalMs / repetitions;
}

int main(int argc, char* argv[]) {
    if (argc != 2) {
        std::cerr << "Usage: " << argv[0]
                  << " data_size\n";
        return 1;
    }

    try {
        const long long parsedSize = std::stoll(argv[1]);

        // Keep size*10 and the int indices representable.
        if (parsedSize <= 0 ||
            parsedSize > std::numeric_limits<int>::max() / 10) {
            throw std::invalid_argument("Invalid data size");
        }

        const int dataSize = static_cast<int>(parsedSize);
        constexpr int repetitions = 100;

        const auto data = generateTestData(dataSize);

        auto sortedData = data;
        std::sort(sortedData.begin(), sortedData.end());

        // Select a target that definitely exists.
        const int target = data[data.size() / 5];

        std::unordered_map<int, int> hashMap;

        // Measure preprocessing separately.
        const auto buildStart = std::chrono::steady_clock::now();
        SearchOptimization::buildHashMap(data, hashMap);
        const auto buildEnd = std::chrono::steady_clock::now();

        const double buildMs =
            std::chrono::duration<double, std::milli>(
                buildEnd - buildStart
            ).count();

        // Validate correctness before benchmarking.
        const int linearIndex =
            SearchOptimization::linearSearch(data, target);

        const int binaryIndex =
            SearchOptimization::binarySearch(sortedData, target);

        const int hashIndex =
            SearchOptimization::hashSearch(hashMap, target);

        if (linearIndex < 0 || data[linearIndex] != target ||
            binaryIndex < 0 || sortedData[binaryIndex] != target ||
            hashIndex < 0 || data[hashIndex] != target) {
            throw std::runtime_error("Search validation failed");
        }

        const double linearMs = benchmarkMs([&] {
            return SearchOptimization::linearSearch(data, target);
        }, repetitions);

        const double binaryMs = benchmarkMs([&] {
            return SearchOptimization::binarySearch(sortedData, target);
        }, repetitions);

        const double hashMs = benchmarkMs([&] {
            return SearchOptimization::hashSearch(hashMap, target);
        }, repetitions);

        std::cout << "Data size: " << dataSize << '\n';
        std::cout << "Target: " << target << '\n';
        std::cout << "Repetitions: " << repetitions << "\n\n";

        std::cout << "Average search time:\n";
        std::cout << "Linear:  " << linearMs << " ms\n";
        std::cout << "Binary:  " << binaryMs << " ms\n";
        std::cout << "Hash:    " << hashMs << " ms\n\n";

        std::cout << "Hash construction time: "
                  << buildMs << " ms\n";

        std::cout << "Hash build + one lookup: "
                  << buildMs + hashMs << " ms\n";
    }
    catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << '\n';
        return 1;
    }

    return 0;
}
