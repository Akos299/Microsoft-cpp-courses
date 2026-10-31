
#include <coroutine>
#include <exception>
#include <optional>
#include <utility>
#include <stdexcept>
#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <sstream>
#include <queue>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <stop_token>
#include <cstdint>
#include <algorithm>

// ============================================================
// 1. CANCELLATION
// ============================================================

class OperationCancelled : public std::runtime_error {
public:
    OperationCancelled()
        : std::runtime_error("Operation cancelled") {}
};

namespace this_coro {
    struct stop_token_t {};
    inline constexpr stop_token_t stop_token{};
}

// ============================================================
// 2. PRIORITY SCHEDULER
// ============================================================

enum class Priority : int {
    Low = 1,
    Normal = 2,
    High = 3
};

class Scheduler {
private:

    struct Job {
        std::coroutine_handle<> handle;
        Priority priority;
        std::uint64_t sequence;
    };

    struct Compare {
        bool operator()(const Job& a, const Job& b) const {

            if (a.priority != b.priority) {
                return static_cast<int>(a.priority)
                     < static_cast<int>(b.priority);
            }

            return a.sequence > b.sequence;
        }
    };

    std::priority_queue<Job, std::vector<Job>, Compare> queue_;
    std::vector<std::thread> workers_;

    std::mutex mutex_;
    std::condition_variable cv_;

    bool stopping_ = false;
    std::uint64_t sequence_ = 0;

    void workerLoop() {

        while (true) {

            Job job;

            {
                std::unique_lock lock(mutex_);

                cv_.wait(lock, [this] {
                    return stopping_ || !queue_.empty();
                });

                if (stopping_ && queue_.empty())
                    return;

                job = queue_.top();
                queue_.pop();
            }

            if (job.handle && !job.handle.done()) {
                job.handle.resume();
            }
        }
    }

public:

    explicit Scheduler(std::size_t numWorkers = 4) {

        numWorkers = std::max<std::size_t>(1, numWorkers);

        for (std::size_t i = 0; i < numWorkers; ++i) {
            workers_.emplace_back([this] {
                workerLoop();
            });
        }
    }

    Scheduler(const Scheduler&) = delete;
    Scheduler& operator=(const Scheduler&) = delete;

    void schedule(
        std::coroutine_handle<> handle,
        Priority priority = Priority::Normal
    ) {

        if (!handle)
            throw std::invalid_argument("Invalid coroutine handle");

        {
            std::lock_guard lock(mutex_);

            if (stopping_)
                throw std::runtime_error("Scheduler stopping");

            queue_.push({
                handle,
                priority,
                sequence_++
            });
        }

        cv_.notify_one();
    }

    ~Scheduler() {

        {
            std::lock_guard lock(mutex_);
            stopping_ = true;
        }

        cv_.notify_all();

        for (auto& worker : workers_) {
            if (worker.joinable())
                worker.join();
        }
    }
};

// ============================================================
// 3. SCHEDULER AWAITER
// ============================================================

class ScheduleOn {

private:
    Scheduler& scheduler_;
    Priority priority_;

public:

    ScheduleOn(Scheduler& scheduler, Priority priority)
        : scheduler_(scheduler),
          priority_(priority) {}

    bool await_ready() const noexcept {
        return false;
    }

    void await_suspend(std::coroutine_handle<> h) {
        scheduler_.schedule(h, priority_);
    }

    void await_resume() const noexcept {}
};

// ============================================================
// 4. TASK<T>
// ============================================================

template<typename T>
class Task {

public:

    struct promise_type;
    using handle_type = std::coroutine_handle<promise_type>;

private:

    handle_type handle_{};
    bool started_ = false;

    explicit Task(handle_type h)
        : handle_(h) {}

    void release() noexcept {

        if (!handle_)
            return;

        if (started_) {

            handle_.promise().stop_source.request_stop();

            try {
                wait();
            }
            catch (...) {
                std::terminate();
            }
        }

        handle_.destroy();
        handle_ = {};
    }

public:

    struct promise_type {

        std::optional<T> value;
        std::exception_ptr exception;

        std::stop_source stop_source;

        mutable std::mutex mutex;
        mutable std::condition_variable cv;

        bool completed = false;

        struct FinalAwaiter {

            bool await_ready() const noexcept {
                return false;
            }

            void await_suspend(handle_type h) const noexcept {

                auto& p = h.promise();

                {
                    std::lock_guard lock(p.mutex);
                    p.completed = true;
                }

                p.cv.notify_all();
            }

            void await_resume() const noexcept {}
        };

        struct StopTokenAwaiter {

            promise_type* promise;

            bool await_ready() const noexcept {
                return true;
            }

            void await_suspend(std::coroutine_handle<>) const noexcept {}

            std::stop_token await_resume() const noexcept {
                return promise->stop_source.get_token();
            }
        };

        Task get_return_object() {

            return Task{
                handle_type::from_promise(*this)
            };
        }

        std::suspend_always initial_suspend() noexcept {
            return {};
        }

        FinalAwaiter final_suspend() noexcept {
            return {};
        }

        void return_value(T result) {
            value = std::move(result);
        }

        void unhandled_exception() {
            exception = std::current_exception();
        }

        auto await_transform(this_coro::stop_token_t) noexcept {
            return StopTokenAwaiter{this};
        }

        template<typename Awaitable>
        auto await_transform(Awaitable&& awaitable) {
            return std::forward<Awaitable>(awaitable);
        }
    };

    Task(const Task&) = delete;
    Task& operator=(const Task&) = delete;

    Task(Task&& other) noexcept
        : handle_(std::exchange(other.handle_, {})),
          started_(std::exchange(other.started_, false)) {}

    Task& operator=(Task&& other) noexcept {

        if (this != &other) {

            release();

            handle_ = std::exchange(other.handle_, {});
            started_ = std::exchange(other.started_, false);
        }

        return *this;
    }

    ~Task() {
        release();
    }

    void start(
        Scheduler& scheduler,
        Priority priority = Priority::Normal
    ) {

        if (!handle_)
            throw std::logic_error("Invalid Task");

        if (started_)
            throw std::logic_error("Task already started");

        started_ = true;

        try {
            scheduler.schedule(handle_, priority);
        }
        catch (...) {
            started_ = false;
            throw;
        }
    }

    void request_stop() {

        if (handle_) {
            handle_.promise().stop_source.request_stop();
        }
    }

    bool is_ready() const {

        if (!handle_)
            return true;

        auto& p = handle_.promise();

        std::lock_guard lock(p.mutex);
        return p.completed;
    }

    void wait() const {

        if (!handle_)
            throw std::logic_error("Invalid Task");

        if (!started_)
            throw std::logic_error("Task not started");

        auto& p = handle_.promise();

        std::unique_lock lock(p.mutex);

        p.cv.wait(lock, [&p] {
            return p.completed;
        });
    }

    T get() {

        wait();

        auto& p = handle_.promise();

        if (p.exception) {
            std::rethrow_exception(p.exception);
        }

        if (!p.value) {
            throw std::logic_error("Task has no result");
        }

        return std::move(*p.value);
    }
};

// ============================================================
// 5. CONFIGURATION MANAGER
// ============================================================

class ConfigManager {

private:

    std::map<std::string, std::string> values_;

public:

    std::string trim(const std::string& str)
    {
        const auto first = std::find_if_not(
            str.begin(), str.end(),
            [](unsigned char c) { return std::isspace(c); }
        );

        if (first == str.end())
            return {};

        const auto last = std::find_if_not(
            str.rbegin(), str.rend(),
            [](unsigned char c) { return std::isspace(c); }
        ).base();

        return std::string(first, last);
    }

    template<typename T>
    T getValue(const std::string& key) const {

        auto it = values_.find(key);

        if (it == values_.end())
            throw std::runtime_error("Missing configuration: " + key);

        std::istringstream stream(it->second);

        T value{};

        if (!(stream >> value))
            throw std::runtime_error("Invalid configuration: " + key);

        stream >> std::ws;

        if (!stream.eof())
            throw std::runtime_error("Invalid configuration: " + key);

        return value;
    }

    std::string getString(
        const std::string& key,
        const std::string& defaultValue
    ) const {

        auto it = values_.find(key);

        return it == values_.end() ? defaultValue : it->second;
    }

    void load(std::istream& input)
    {
        std::string line;

        while (std::getline(input, line))
        {
            // Remove leading and trailing whitespace
            line = trim(line);

            // Ignore empty lines and comments
            if (line.empty() || line[0] == '#')
                continue;

            const auto separator = line.find('=');

            if (separator == std::string::npos)
                continue;

            // Extract and trim both key and value
            std::string key = trim(line.substr(0, separator));
            std::string value = trim(line.substr(separator + 1));

            if (key.empty())
                continue;

            values_[key] = value;

            std::cout << "Loaded: [" << key
                    << "] = [" << value << "]\n";
        }
    }
};

// ============================================================
// 6. VALIDATED RUNTIME CONFIGURATION
// ============================================================

struct RuntimeConfig {

    int numThreads = 4;
    int maxRecords = 10000;
    std::string backend = "cpu";
};

RuntimeConfig buildRuntimeConfig(const ConfigManager& config) {

    RuntimeConfig result;

    result.numThreads = config.getValue<int>("num_threads");
    result.maxRecords = config.getValue<int>("max_records");
    result.backend = config.getString("backend", "cpu");

    if (result.numThreads <= 0)
        throw std::runtime_error("num_threads must be positive");

    if (result.maxRecords <= 0)
        throw std::runtime_error("max_records must be positive");

    if (result.backend != "cpu" && result.backend != "gpu")
        throw std::runtime_error("Unsupported backend");

    return result;
}

// ============================================================
// 7. DATA PROCESSOR
// ============================================================

class DataProcessor {

public:

    std::vector<std::string> filterNonEmptyStrings(
        const std::vector<std::string>& input
    ) const {

        std::vector<std::string> output;

        for (const auto& value : input) {

            if (!value.empty()) {
                output.push_back(value);
            }
        }

        return output;
    }

    template<typename Function>
    std::vector<std::string> transformStrings(
        const std::vector<std::string>& input,
        Function function
    ) const {

        std::vector<std::string> output;
        output.reserve(input.size());

        for (const auto& value : input) {
            output.push_back(function(value));
        }

        return output;
    }

    std::size_t countCharacter(
        const std::string& value,
        char character
    ) const {

        return static_cast<std::size_t>(
            std::count(value.begin(), value.end(), character)
        );
    }
};

// ============================================================
// 8. CANCELLATION CHECKPOINT
// ============================================================

void checkCancellation(std::stop_token token) {

    if (token.stop_requested()) {
        throw OperationCancelled{};
    }
}

// ============================================================
// 9. ASYNCHRONOUS DATA PIPELINE
// ============================================================

Task<std::vector<std::string>> processPipeline(
    Scheduler& scheduler,
    const DataProcessor& processor,
    std::vector<std::string> input,
    RuntimeConfig config,
    int taskId
) {

    auto token = co_await this_coro::stop_token;

    std::cout << "Task " << taskId << ": started\n";

    // ---------------------------
    // Stage 1: Filtering
    // ---------------------------

    checkCancellation(token);

    if (input.size() > static_cast<std::size_t>(config.maxRecords)) {
        throw std::runtime_error("Input exceeds max_records");
    }

    auto filtered = processor.filterNonEmptyStrings(input);

    co_await ScheduleOn(scheduler, Priority::Normal);

    // ---------------------------
    // Stage 2: Transformation
    // ---------------------------

    checkCancellation(token);

    auto transformed = processor.transformStrings(
        filtered,
        [&config](const std::string& value) {
            return "[" + config.backend + "] " + value;
        }
    );

    co_await ScheduleOn(scheduler, Priority::Normal);

    // ---------------------------
    // Stage 3: Computation
    // ---------------------------

    checkCancellation(token);

    std::vector<std::string> results;

    results.reserve(transformed.size());

    for (const auto& value : transformed) {

        checkCancellation(token);

        auto count = processor.countCharacter(value, '_');

        results.push_back(
            value + " | underscore_count=" + std::to_string(count)
        );
    }

    std::cout << "Task " << taskId << ": completed\n";

    co_return results;
}

// ============================================================
// 10. MAIN APPLICATION
// ============================================================

int main() {

    try {

        // Configuration
        ConfigManager configManager;

        std::istringstream configuration(R"(
            num_threads=4
            max_records=10000
            backend=cpu
        )");

        configManager.load(configuration);

        RuntimeConfig config =
            buildRuntimeConfig(configManager);

        // Scheduler
        Scheduler scheduler(config.numThreads);

        // Data processor
        DataProcessor processor;

        // Input datasets
        std::vector<std::string> data1 = {
            "particle_1",
            "",
            "particle_2",
            "particle_3"
        };

        std::vector<std::string> data2 = {
            "mesh_cell_1",
            "mesh_cell_2",
            "",
            "mesh_cell_3"
        };

        // Create tasks
        auto task1 = processPipeline(
            scheduler,
            processor,
            std::move(data1),
            config,
            1
        );

        auto task2 = processPipeline(
            scheduler,
            processor,
            std::move(data2),
            config,
            2
        );

        // Submit with different priorities
        task1.start(scheduler, Priority::High);
        task2.start(scheduler, Priority::Normal);

        // Retrieve results
        auto result1 = task1.get();
        auto result2 = task2.get();

        std::cout << "\n========== RESULTS ==========\n";

        for (const auto& value : result1) {
            std::cout << value << '\n';
        }

        for (const auto& value : result2) {
            std::cout << value << '\n';
        }

        std::cout << "\nAll pipelines completed.\n";
    }
    catch (const OperationCancelled& e) {
        std::cerr << "Cancelled: " << e.what() << '\n';
        return 2;
    }
    catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << '\n';
        return 1;
    }

    return 0;
}
