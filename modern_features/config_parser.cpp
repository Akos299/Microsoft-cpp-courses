#include <iostream>
#include <optional>
#include <variant>
#include <map>
#include <string>
#include <fstream>
#include <sstream>
#include <vector>

class ConfigManager {
private:
    using ConfigValue = std::variant<int, double, std::string, bool>;
    std::map<std::string, ConfigValue> settings;

public:
    // Load configuration from key-value pairs
    void loadConfig(const std::vector<std::string>& configLines) {
        for (const auto& line : configLines) {
            auto [key, value] = parseConfigLine(line);
            if ( key && value) {
                settings[*key] = *value;
            }
            else{
                std::cerr << "Invalid configuration: " << line << "\n";
            }
        }
    }

    // Template method with if constexpr for type-safe retrieval
    template<typename T>
    std::optional<T> getValue(const std::string& key) const {
        auto it = settings.find(key);
        if (it == settings.end()) {
            return std::nullopt;
        }

        if constexpr (std::is_same_v<T, int>) {
            if (std::holds_alternative<int>(it->second)) {
                return std::get<int>(it->second);
            }
        } else if constexpr (std::is_same_v<T, double>) {
            if (std::holds_alternative<double>(it->second)) {
                return std::get<double>(it->second);
            }
        } else if constexpr (std::is_same_v<T, std::string>) {
            if (std::holds_alternative<std::string>(it->second)) {
                return std::get<std::string>(it->second);
            }
        } else if constexpr (std::is_same_v<T, bool>) {
            if (std::holds_alternative<bool>(it->second)) {
                return std::get<bool>(it->second);
            }
        }
        return std::nullopt;
    }

    // Template method to fall back if key exits but not the value provided in configuration file

    template<typename T>
    std::optional<T> getValueOr(const std::string& key, const T& defaultValue) const {
        auto key_value =  getValue<T>(key);
        return  key_value.value_or(defaultValue);
    }


    template<typename  T, typename Validator>
    bool validate(const std::string& key, Validator validator_, const std::string& errorMsg){
        if(!contains(key)){
            std::cerr << "Missing parameter: " << key << '\n';
            return false;

        }
        auto value = getValueOr<T>(key);

        if (!value) {
            std::cerr << "Incorrect type: " << key << '\n';
            return false;
        }

        if(!validator_(value.value())){
            std::cerr << "Invalid parameter: "
                << key << " - " << errorMsg << '\n';
            return false;
        }

        return true;
    }

private:
    bool contains(const std::string& key) const {
        return settings.find(key) != settings.end();
    }
    // Use structured bindings for clean parsing
    std::pair<std::optional<std::string>, std::optional<ConfigValue>> 
    parseConfigLine(const std::string& line) {
        // Implement parsing logic here
        // Return {key, value} pair using structured bindings

        // Find '=' separator
        const auto separator = line.find("=");
        if(separator == std::string::npos){
            return {std::nullopt, std::nullopt};
        }

        std::string key = line.substr(0, separator);
        std::string value = line.substr(separator+1);

        // Trim white space
        auto trim = [](std::string& str) {
            // find the first charater different from white space
            const auto first = str.find_first_not_of(" \t\r\n");

            // if the string only contains white spaces, then empty it.
            if (first == std::string::npos) {
                str.clear();
                return;
            }

            // find the last character different from white space
            const auto last = str.find_last_not_of(" \t\r\n");
            str = str.substr(first, last - first + 1);
        };

        trim(key);
        trim(value);

        if (key.empty() || value.empty())
            return {std::nullopt, std::nullopt};

        // Parse boolean
        if (value == "true")
            return {key, ConfigValue{true}};

        if (value == "false")
            return {key, ConfigValue{false}};

        // Parse integer
        try {
            std::size_t pos = 0;
            int intValue = std::stoi(value, &pos);

            if (pos == value.size())
                return {key, ConfigValue{intValue}};
        }
        catch (...) {
            // Not an integer: try another type
        }

        // Parse double
        try {
            std::size_t pos = 0;
            double doubleValue = std::stod(value, &pos);

            if (pos == value.size())
                return {key, ConfigValue{doubleValue}};
        }
        catch (...) {
            // Not a double: treat as string
        }

        return {key, ConfigValue(value)};
    }
};


// Test program
int main() {

    ConfigManager config;

    std::vector<std::string> configLines = {
        "database_port=5432",
        "app_name=MyApp",
        "debug_mode=true",
        "simulation_dt=0.001",
        "max_iterations=1000",
        "use_gpu=false",
        "backend =HIP ",
        "invalid_line",
        "empty_value="
    };

    config.loadConfig(configLines);

    // Retrieve values
    auto port = config.getValueOr<int>("database_port", 1200);
    auto app = config.getValueOr<std::string>("app_name", "FEM_CODE");
    auto debug = config.getValueOr<int>("debug_mode", false);
    auto dt = config.getValueOr<double>("simulation_dt", 1e-5);
    auto iterations = config.getValueOr<int>("max_iterations", 100);
    auto gpu = config.getValueOr<bool>("use_gpu", false);
    auto backend= config.getValueOr<std::string>("backend", "Serial");
    if (port)
        std::cout << "Database port: " << *port << '\n';

    if (app)
        std::cout << "Application: " << *app << '\n';

    if (debug)
        std::cout << "Debug mode: " << std::boolalpha << *debug << '\n';

    if (dt)
        std::cout << "Time step: " << *dt << '\n';

    if (iterations)
        std::cout << "Max iterations: " << *iterations << '\n';

    if (gpu)
        std::cout << "GPU enabled: " << std::boolalpha << *gpu << '\n';
    
    if(backend)
        std::cout << "Backend: " << backend.value() << "\n";

    // Wrong requested type
    auto wrongType = config.getValue<double>("database_port");

    if (!wrongType)
        std::cout << "Type mismatch detected!\n";

    return 0;
}