
#include <algorithm>
#include <charconv>
#include <cmath>
#include <concepts>
#include <exception>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <map>
#include <memory>
#include <numeric>
#include <optional>
#include <ranges>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <variant>
#include <coroutine>
#include <vector>

// ============================================================
// 1. Custom exceptions
// ============================================================

class ApplicationError : public std::runtime_error {
public:
    using std::runtime_error::runtime_error;
};

class ConfigurationError : public ApplicationError {
public:
    using ApplicationError::ApplicationError;
};

class DataProcessingError : public ApplicationError {
public:
    using ApplicationError::ApplicationError;
};

// ============================================================
// 2. C++20 Task<T> coroutine abstraction
// ============================================================

template<typename T>
class Task {
public:
    struct promise_type;

    using Handle = std::coroutine_handle<promise_type>;

    struct promise_type {
        std::optional<T> result;
        std::exception_ptr exception;
        std::coroutine_handle<> continuation =
            std::noop_coroutine();

        Task get_return_object() {
            return Task{Handle::from_promise(*this)};
        }

        std::suspend_always initial_suspend() noexcept {
            return {};
        }

        struct FinalAwaiter {
            bool await_ready() noexcept {
                return false;
            }

            std::coroutine_handle<> await_suspend(
                Handle handle) noexcept {
                return handle.promise().continuation;
            }

            void await_resume() noexcept {}
        };

        FinalAwaiter final_suspend() noexcept {
            return {};
        }

        void return_value(T value) {
            result.emplace(std::move(value));
        }

        void unhandled_exception() noexcept {
            exception = std::current_exception();
        }
    };

    struct Awaiter {
        Handle handle;

        bool await_ready() const noexcept {
            return !handle || handle.done();
        }

        std::coroutine_handle<> await_suspend(
            std::coroutine_handle<> parent) noexcept {
            handle.promise().continuation = parent;
            return handle;
        }

        T await_resume() {
            if (handle.promise().exception) {
                std::rethrow_exception(
                    handle.promise().exception);
            }

            return std::move(*handle.promise().result);
        }

        ~Awaiter() {
            if (handle) {
                handle.destroy();
            }
        }
    };

    explicit Task(Handle handle) : handle_(handle) {}

    Task(const Task&) = delete;
    Task& operator=(const Task&) = delete;

    Task(Task&& other) noexcept
        : handle_(std::exchange(other.handle_, {})) {}

    Task& operator=(Task&& other) noexcept {
        if (this != &other) {
            if (handle_) {
                handle_.destroy();
            }

            handle_ = std::exchange(other.handle_, {});
        }

        return *this;
    }

    ~Task() {
        if (handle_) {
            handle_.destroy();
        }
    }

    Awaiter operator co_await() && {
        return Awaiter{
            std::exchange(handle_, {})
        };
    }

    T get() {
        if (!handle_) {
            throw ApplicationError("Invalid task");
        }

        while (!handle_.done()) {
            handle_.resume();
        }

        if (handle_.promise().exception) {
            std::rethrow_exception(
                handle_.promise().exception);
        }

        return std::move(*handle_.promise().result);
    }

private:
    Handle handle_;
};

// ============================================================
// 3. Configuration management
// ============================================================

// A configuration value can hold several different types.
using ConfigValue = std::variant<
    int,
    double,
    bool,
    std::string
>;

class ConfigManager {
private:
    std::map<std::string, ConfigValue> settings;

    static std::string trim(std::string value) {
        const auto first = value.find_first_not_of(" \t\r\n");

        if (first == std::string::npos) {
            return {};
        }

        const auto last = value.find_last_not_of(" \t\r\n");

        return value.substr(first, last - first + 1);
    }

    static ConfigValue parseValue(const std::string& value) {
        if (value == "true") return true;
        if (value == "false") return false;

        int integer{};

        const auto [ptr, ec] = std::from_chars(
            value.data(),
            value.data() + value.size(),
            integer
        );

        if (ec == std::errc{} &&
            ptr == value.data() + value.size()) {
            return integer;
        }

        try {
            std::size_t position = 0;
            double number = std::stod(value, &position);

            if (position == value.size() &&
                std::isfinite(number)) {
                return number;
            }
        }
        catch (const std::exception&) {
            // Not a valid numeric value: keep it as a string.
        }

        return value;
    }

public:
    static ConfigManager load(const std::string& filename) {
        if (!std::filesystem::exists(filename)) {
            throw ConfigurationError(
                "Configuration file does not exist: " + filename
            );
        }

        std::ifstream file(filename);

        if (!file) {
            throw ConfigurationError(
                "Cannot open configuration file: " + filename
            );
        }

        ConfigManager config;
        std::string line;
        std::size_t lineNumber = 0;

        while (std::getline(file, line)) {
            ++lineNumber;

            line = trim(line);

            if (line.empty() || line.starts_with("#")) {
                continue;
            }

            const auto separator = line.find('=');

            if (separator == std::string::npos) {
                throw ConfigurationError(
                    "Invalid configuration at line " +
                    std::to_string(lineNumber)
                );
            }

            // Structured bindings (C++17).
            auto [key, value] = std::pair{
                trim(line.substr(0, separator)),
                trim(line.substr(separator + 1))
            };

            if (key.empty() || value.empty()) {
                throw ConfigurationError(
                    "Empty key or value at line " +
                    std::to_string(lineNumber)
                );
            }

            if (config.settings.contains(key)) {
                throw ConfigurationError(
                    "Duplicate configuration key: " + key
                );
            }

            config.settings.emplace(
                std::move(key),
                parseValue(value)
            );
        }

        if (file.bad()) {
            throw ConfigurationError(
                "Error while reading configuration file"
            );
        }

        return config;
    }

    std::optional<ConfigValue> get(
        const std::string& key) const {

        auto it = settings.find(key);

        if (it == settings.end()) {
            return std::nullopt;
        }

        return it->second;
    }

    template<typename T>
    T getOr(const std::string& key, T defaultValue) const {
        auto value = get(key);

        if (!value) {
            return defaultValue;
        }

        if (const auto* result = std::get_if<T>(&*value)) {
            return *result;
        }

        throw ConfigurationError(
            "Incorrect type for configuration key: " + key
        );
    }

    void print() const {
        std::cout << "\nConfiguration:\n";

        for (const auto& [key, value] : settings) {
            std::cout << "  " << key << " = ";

            std::visit(
                [](const auto& item) {
                    std::cout << std::boolalpha << item;
                },
                value
            );

            std::cout << '\n';
        }
    }
};

// ============================================================
// 4. Concepts and generic data processing
// ============================================================

template<typename T>
concept Numeric = std::convertible_to<T, double>;

template<typename R>
concept NumericRange =
    std::ranges::input_range<R> &&
    Numeric<std::ranges::range_value_t<R>>;

class DataProcessor {
public:
    template<NumericRange Range>
    std::vector<double> process(
        Range&& input,
        double threshold,
        bool removeNegative) const {

        auto validValues = std::forward<Range>(input)
            | std::views::filter(
                [threshold, removeNegative](const auto& value) {
                    const double x = static_cast<double>(value);

                    return std::isfinite(x) &&
                           x >= threshold &&
                           (!removeNegative || x >= 0.0);
                }
            );

        std::vector<double> results;

        for (const auto& value : validValues) {
            results.push_back(
                static_cast<double>(value)
            );
        }

        return results;
    }

    std::vector<double> loadAndProcess(
        const std::string& filename,
        double threshold,
        bool removeNegative) const {

        std::ifstream file(filename);

        if (!file) {
            throw DataProcessingError(
                "Cannot open data file: " + filename
            );
        }

        std::vector<double> values;
        std::string line;

        while (std::getline(file, line)) {
            if (line.empty()) {
                continue;
            }

            try {
                std::size_t position = 0;
                double value = std::stod(line, &position);

                if (position != line.size()) {
                    throw DataProcessingError(
                        "Unexpected characters in data: " + line
                    );
                }

                values.push_back(value);
            }
            catch (const DataProcessingError&) {
                throw;
            }
            catch (const std::exception&) {
                throw DataProcessingError(
                    "Invalid numerical value: " + line
                );
            }
        }

        if (file.bad()) {
            throw DataProcessingError(
                "Error while reading data file"
            );
        }

        return process(
            values,
            threshold,
            removeNegative
        );
    }
};

// ============================================================
// 5. Task manager
// ============================================================

class AsyncTaskManager {
public:
    template<typename T>
    T wait(Task<T> task) const {
        return task.get();
    }
};

// ============================================================
// 6. Complete application
// ============================================================

class ModernCppApplication {
private:
    std::unique_ptr<ConfigManager> configManager;
    std::unique_ptr<DataProcessor> dataProcessor;
    std::unique_ptr<AsyncTaskManager> taskManager;

    // Configuration loading as a coroutine.
    Task<ConfigManager> loadConfigurationAsync(
        const std::string& filename) {

        co_return ConfigManager::load(filename);
    }

    // Process data using configuration parameters.
    std::vector<double> processDataWithModernFeatures(
        const std::string& filename,
        const ConfigManager& config) {

        const double threshold =
            config.getOr<double>("threshold", 0.0);

        const bool removeNegative =
            config.getOr<bool>("remove_negative", true);

        return dataProcessor->loadAndProcess(
            filename,
            threshold,
            removeNegative
        );
    }

    // Generate a statistical report.
    std::string generateReport(
        const ConfigManager& config,
        const std::vector<double>& results) {

        std::ostringstream report;

        report << "\n========== SCIENTIFIC DATA REPORT ==========\n";

        report << "\nBackend: "
               << config.getOr<std::string>("backend", "cpu");

        report << "\nThreads: "
               << config.getOr<int>("num_threads", 1);

        report << "\nProcessed values: "
               << results.size() << '\n';

        if (!results.empty()) {
            const double sum = std::accumulate(
                results.begin(),
                results.end(),
                0.0
            );

            const double average =
                sum / static_cast<double>(results.size());

            const auto [minimum, maximum] =
                std::ranges::minmax(results);

            report << std::fixed << std::setprecision(4);

            report << "\nStatistics:\n";
            report << "  Minimum: " << minimum << '\n';
            report << "  Maximum: " << maximum << '\n';
            report << "  Average: " << average << '\n';
            report << "  Sum:     " << sum << '\n';
        }
        else {
            report << "\nNo valid values were found.\n";
        }

        report << "\n============================================\n";

        return report.str();
    }

public:
    ModernCppApplication()
        : configManager(std::make_unique<ConfigManager>()),
          dataProcessor(std::make_unique<DataProcessor>()),
          taskManager(std::make_unique<AsyncTaskManager>()) {}

    // Prevent accidental copying of ownership.
    ModernCppApplication(const ModernCppApplication&) = delete;
    ModernCppApplication& operator=(
        const ModernCppApplication&) = delete;

    // Main coroutine workflow.
    Task<std::string> runCompleteWorkflow(
        const std::string& configFile,
        const std::string& dataFile) {

        try {
            // 1. Load configuration.
            ConfigManager config =
                co_await loadConfigurationAsync(configFile);

            // 2. Display configuration.
            config.print();

            // 3. Process data.
            auto processingResults =
                processDataWithModernFeatures(dataFile, config);

            // 4. Generate report.
            co_return generateReport(
                config,
                processingResults
            );
        }
        catch (...) {
            std::throw_with_nested(
                ApplicationError("Workflow execution failed")
            );
        }
    }

    // Application entry point.
    void run(
        const std::string& configFile,
        const std::string& dataFile) {

        try {
            std::string report = taskManager->wait(
                runCompleteWorkflow(configFile, dataFile)
            );

            std::cout << report;
        }
        catch (const std::exception& error) {
            std::cerr << "\nAPPLICATION ERROR: "
                      << error.what() << '\n';

            // Print nested exceptions, if present.
            try {
                std::rethrow_if_nested(error);
            }
            catch (const std::exception& nested) {
                std::cerr << "Caused by: "
                          << nested.what() << '\n';
            }
        }
    }
};

// ============================================================
// 7. Main
// ============================================================

int main() {
    try {
        ModernCppApplication application;

        application.run(
            "config.txt",
            "data.txt"
        );
    }
    catch (const std::exception& error) {
        std::cerr << "Fatal error: "
                  << error.what() << '\n';

        return 1;
    }

    return 0;
}
