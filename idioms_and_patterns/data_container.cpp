#include <vector>
#include <algorithm>
#include <type_traits>
#include <utility>
#include <iostream>
#include <functional>

template<typename T>
class ModernContainer {
private:
    std::vector<T> data;
    
public:
    ModernContainer() = default;
    
    // Constructor with initializer list
    ModernContainer(std::initializer_list<T> init) : data(init) {}
    
    // Implement perfect forwarding for adding elements
    template<typename U>
    void emplace(U&& item) {
        // Use perfect forwarding to preserve value category
        data.emplace_back(std::forward<U>(item));
    }
    
    // Implement variadic template for in-place construction
    template<typename... Args>
    void emplaceConstruct(Args&&... args) {
        // Forward all arguments for in-place construction
        data.emplace_back(std::forward<Args>(args)...);
    }
    
    // Modern iteration support
    auto begin() { return data.begin(); }
    auto end() { return data.end(); }
    auto begin() const { return data.begin(); }
    auto end() const { return data.end(); }
    
    // Implement modern search with auto return type
    auto findElement(const T& target) {
        // Use std::find and return iterator
        return std::find(data.begin(), data.end(), target);
    }
    
    // Apply modern algorithms with lambda support
    void forEach(const std::function<void(const T&)>& func) {
        // Use std::for_each algorithm
        std::for_each(data.begin(), data.end(), [func](T& val){func(std::forward<T>(val));});
    }
    
    // Implement filtering with lambda predicates
    std::vector<T> filter(const std::function<bool(const T&)>& predicate) {
        std::vector<T> result;
        // Use std::copy_if algorithm
        std::copy_if(data.begin(), data.end(), std::back_inserter(result), predicate);
        return result;
    }
    
    size_t size() const { return data.size(); }
    bool empty() const { return data.empty(); }
};

// Test class for perfect forwarding
class TestObject {
private:
    std::string value;
    
public:
    TestObject(const std::string& v) : value(v) {
        std::cout << "TestObject constructed from lvalue: " << value << std::endl;
    }
    
    TestObject(std::string&& v) : value(std::move(v)) {
        std::cout << "TestObject constructed from rvalue: " << value << std::endl;
    }
    
    TestObject(const std::string& a, const std::string& b) : value(a + "-" + b) {
        std::cout << "TestObject constructed with two args: " << value << std::endl;
    }
    
    const std::string& getValue() const { return value; }
};


int main() {
    ModernContainer<TestObject> objects;

    std::cout << "\n=== Test 1: Forward an lvalue ===\n";
    std::string lvalue = "Hello";
    objects.emplace(lvalue);

    std::cout << "\n=== Test 2: Forward an rvalue ===\n";
    objects.emplace(std::string("World"));

    std::cout << "\n=== Test 3: In-place construction ===\n";
    objects.emplaceConstruct(std::string("Modern"),
                             std::string("C++"));

    std::cout << "\n=== Test 4: Display all objects ===\n";
    objects.forEach([](const TestObject& obj) {
        std::cout << "Value: " << obj.getValue() << '\n';
    });

    std::cout << "\n=== Test 5: Container information ===\n";
    std::cout << "Size: " << objects.size() << '\n';
    std::cout << "Empty: " << std::boolalpha
              << objects.empty() << '\n';

    return 0;
}