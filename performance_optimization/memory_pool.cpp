#include <chrono>
#include <cstddef>
#include <cstdint>
#include <iomanip>
#include <iostream>
#include <memory>
#include <new>
#include <random>
#include <stdexcept>
#include <utility>
#include <vector>
#include <algorithm>
#include <cmath>

template<typename T>
class CustomMemoryPool {
private:
    static constexpr std::size_t npos = static_cast<std::size_t>(-1);

    T* storage_ = nullptr;
    std::size_t capacity_ = 0;
    std::size_t freeHead_ = npos;
    std::size_t available_ = 0;

    // nextFree_[i] is the next free slot after i.
    std::vector<std::size_t> nextFree_;
    std::vector<bool> used_;
    std::allocator<T> allocator_;

    // Validate pointer membership before pointer arithmetic.
    // Comparing unrelated raw pointers with relational operators is not
    // portable, so compare their integer representations instead.
    std::size_t indexOf(const T* ptr) const {
        if (ptr == nullptr || storage_ == nullptr) {
            throw std::invalid_argument("Null or invalid pool pointer");
        }

        const auto base = reinterpret_cast<std::uintptr_t>(storage_);
        const auto address = reinterpret_cast<std::uintptr_t>(ptr);
        const std::size_t bytes = capacity_ * sizeof(T);

        if (address < base || address >= base + bytes) {
            throw std::invalid_argument(
                "Pointer does not belong to this pool");
        }

        const auto offset = address - base;
        if (offset % sizeof(T) != 0) {
            throw std::invalid_argument(
                "Pointer does not point to a pool slot");
        }

        return static_cast<std::size_t>(offset / sizeof(T));
    }

public:
    explicit CustomMemoryPool(std::size_t poolSize)
        : capacity_(poolSize),
          available_(poolSize),
          nextFree_(poolSize, npos),
          used_(poolSize, false)
    {
        if (poolSize == 0) {
            throw std::invalid_argument(
                "Pool size must be greater than zero");
        }

        storage_ = allocator_.allocate(capacity_);

        for (std::size_t i = 0; i < capacity_; ++i) {
            nextFree_[i] = (i + 1 < capacity_) ? i + 1 : npos;
        }

        freeHead_ = 0;
    }

    CustomMemoryPool(const CustomMemoryPool&) = delete;
    CustomMemoryPool& operator=(const CustomMemoryPool&) = delete;
    CustomMemoryPool(CustomMemoryPool&&) = delete;
    CustomMemoryPool& operator=(CustomMemoryPool&&) = delete;

    ~CustomMemoryPool() {
        // Safety net: destroy objects that were not explicitly deallocated.
        for (std::size_t i = 0; i < capacity_; ++i) {
            if (used_[i]) {
                std::destroy_at(storage_ + i);
            }
        }

        if (storage_ != nullptr) {
            allocator_.deallocate(storage_, capacity_);
        }
    }

    template<typename... Args>
    T* allocate(Args&&... args) {
        if (freeHead_ == npos) {
            throw std::bad_alloc();
        }

        const std::size_t index = freeHead_;
        freeHead_ = nextFree_[index];

        try {
            T* ptr = std::__construct_at(
                storage_ + index, std::forward<Args>(args)...);

            used_[index] = true;
            --available_;
            return ptr;
        } catch (...) {
            // Restore the free-list head if construction throws.
            nextFree_[index] = freeHead_;
            freeHead_ = index;
            throw;
        }
    }

    void deallocate(T* ptr) {
        if (ptr == nullptr) {
            return;
        }

        const std::size_t index = indexOf(ptr);

        if (!used_[index]) {
            throw std::invalid_argument(
                "Double free or slot is not allocated");
        }

        std::destroy_at(ptr);
        used_[index] = false;

        // Push the released slot onto the front of the free list.
        nextFree_[index] = freeHead_;
        freeHead_ = index;
        ++available_;
    }

    std::size_t availableSlots() const noexcept {
        return available_;
    }

    std::size_t capacity() const noexcept {
        return capacity_;
    }
};

struct TestObject {
    double data[8];
    int id;

    explicit TestObject(int id_val) : data{}, id(id_val) {
        for (int i = 0; i < 8; ++i) {
            data[i] = static_cast<double>(id_val) * i;
        }
    }
};

template<typename Function>
double benchmarkMs(Function&& function, int repetitions) {
    function(); // Warm-up

    const auto start = std::chrono::steady_clock::now();

    for (int i = 0; i < repetitions; ++i) {
        function();
    }

    const auto stop = std::chrono::steady_clock::now();

    return std::chrono::duration<double, std::milli>(
        stop - start).count() / repetitions;
}

int main() {
    constexpr std::size_t POOL_SIZE = 1000;
    constexpr int CYCLES = 1000;

    CustomMemoryPool<TestObject> pool(POOL_SIZE);
    std::vector<TestObject*> poolObjects;
    std::vector<TestObject*> heapObjects;

    poolObjects.reserve(POOL_SIZE);
    heapObjects.reserve(POOL_SIZE);

    volatile long long checksum = 0;

    // 1. Correctness: the pool should initially be empty of live objects.
    if (pool.availableSlots() != POOL_SIZE) {
        std::cerr << "FAIL: initial pool capacity is incorrect\n";
        return 1;
    }

    // 2. Correctness: fill the pool, verify exhaustion, then release all.
    try {
        for (std::size_t i = 0; i < POOL_SIZE; ++i) {
            poolObjects.push_back(pool.allocate(static_cast<int>(i)));
        }

        if (pool.availableSlots() != 0) {
            std::cerr << "FAIL: expected pool to be full\n";
            return 1;
        }

        bool exhaustionCaught = false;
        try {
            (void)pool.allocate(POOL_SIZE);
        } catch (const std::bad_alloc&) {
            exhaustionCaught = true;
        }

        if (!exhaustionCaught) {
            std::cerr << "FAIL: pool exhaustion was not detected\n";
            return 1;
        }

        for (std::size_t i = 0; i < POOL_SIZE; ++i) {
            if (poolObjects[i]->id != static_cast<int>(i)) {
                std::cerr << "FAIL: object contents are incorrect\n";
                return 1;
            }
        }

        for (TestObject* ptr : poolObjects) {
            pool.deallocate(ptr);
        }
        poolObjects.clear();

        if (pool.availableSlots() != POOL_SIZE) {
            std::cerr << "FAIL: pool slots were not all returned\n";
            return 1;
        }
    } catch (const std::exception& e) {
        std::cerr << "Correctness test failed: " << e.what() << '\n';
        return 1;
    }

    std::cout << "Pool correctness tests: PASS\n";
    std::cout << "  Initial free slots: " << pool.availableSlots() << '\n';
    std::cout << "  Exhaustion detection: PASS\n";
    std::cout << "  Construction/data check: PASS\n";
    std::cout << "  All slots returned: PASS\n\n";

    // 3. Benchmark heap allocation/deallocation.
    const double heapMs = benchmarkMs([&]() {
        for (std::size_t i = 0; i < POOL_SIZE; ++i) {
            heapObjects.push_back(new TestObject(static_cast<int>(i)));
        }

        for (TestObject* ptr : heapObjects) {
            checksum = checksum + ptr->id;
            delete ptr;
        }

        heapObjects.clear();
    }, CYCLES);

    // 4. Benchmark pool allocation/deallocation.
    const double poolMs = benchmarkMs([&]() {
        for (std::size_t i = 0; i < POOL_SIZE; ++i) {
            poolObjects.push_back(pool.allocate(static_cast<int>(i)));
        }

        for (TestObject* ptr : poolObjects) {
            checksum = checksum + ptr->id;
            pool.deallocate(ptr);
        }

        poolObjects.clear();
    }, CYCLES);

    // 5. Final sanity checks.
    if (pool.availableSlots() != POOL_SIZE) {
        std::cerr << "FAIL: slots remain allocated after benchmark\n";
        return 1;
    }

    std::cout << std::fixed << std::setprecision(3);
    std::cout << "========== Benchmark ==========\n";
    std::cout << "Objects per cycle: " << POOL_SIZE << '\n';
    std::cout << "Cycles: " << CYCLES << '\n';
    std::cout << "Allocations per method: "
              << static_cast<std::size_t>(POOL_SIZE) * CYCLES << '\n';
    std::cout << "Deallocations per method: "
              << static_cast<std::size_t>(POOL_SIZE) * CYCLES << "\n\n";

    std::cout << "Heap new/delete: "
              << heapMs << " ms per cycle\n";
    std::cout << "Pool allocate/deallocate: "
              << poolMs << " ms per cycle\n";

    if (poolMs > 0.0) {
        std::cout << "Speedup (heap / pool): "
                  << heapMs / poolMs << "x\n";
    }

    std::cout << "\nFinal available slots: "
              << pool.availableSlots() << " / " << pool.capacity() << '\n';
    std::cout << "Checksum (keep work observable): " << checksum << '\n';

    std::cout << "\nFragmentation note:\n"
              << "This test does not measure fragmentation directly. The pool\n"
              << "keeps fixed-size slots in one preallocated region, which can\n"
              << "reduce external fragmentation for same-sized objects.\n";

    return 0;
}
