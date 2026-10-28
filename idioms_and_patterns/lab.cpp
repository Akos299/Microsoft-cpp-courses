// ============================================================================
// Modern C++ Idioms and Patterns Lab - Starter Files
// Overview: Learn RAII principles, move semantics, and modern design patterns
// Duration: 80 minutes
// Instructions: Follow the TODOs to implement modern C++ concepts
// ============================================================================

#include <cstddef>
#include <ios>
#include <iostream>
#include <iterator>
#include <memory>
#include <stdexcept>
#include <vector>
#include <string>
#include <fstream>
#include <functional>
#include <algorithm>
#include <chrono>
#include <utility>

using namespace std;

// ============================================================================
// PART 1: RAII Principles and Resource Management
// ============================================================================

// TODO 1: Implement RAII file handler
class FileHandler {
private:
    std::unique_ptr<std::fstream> file;
    std::string filename;
    
public:
    FileHandler(const string& filename): filename(filename), file(std::make_unique<std::fstream>()) {
        // 1. Open the file using std::fstream
        // 2. Check if file opened successfully
        // 3. Throw exception if file cannot be opened
        file->open(filename, std::ios::in | std::ios::out | std::ios::trunc );
        if(!file->is_open()){
            throw std::runtime_error("Failed to open file: " + filename);
        }
        std::cout << "File opened: " << filename << std::endl;
    }

    // Release the resource automatically.
    ~FileHandler() noexcept{
        // RAII ensures automatic cleanup
        // Close file if it's still open
        if(file && file->is_open()){
            file->close();
            cout << "File automatically closed: " << filename << std::endl;
        }
    }
    
    // Delete copy constructor and assignment operator

    // A file handler has unique ownership.
    FileHandler(const FileHandler&) = delete;
    FileHandler& operator=(const FileHandler&) = delete;

    // Ownership can be transferred.
    FileHandler(FileHandler&& other) noexcept : file(std::move(other.file)), filename(std::move(other.filename)) {}
    FileHandler& operator=(FileHandler&& other) noexcept {
        if(this != &other){
            if(file && file->is_open()){
                file->close();
            }

            file = std::move(other.file);
            filename = std::move(other.filename);
        }
        return *this;
    };

    
    void writeLine(const string& text) {
        // Write text to file with newline
        if(!file->is_open()){
            throw std::runtime_error("File is not open for writing");
        }
        file->clear();                                             // Clear previous stream errors
        file->seekp(0, std::ios::end);                   //  Movw write position to EOF

        *file << text << "\n";
        file->flush();

        if(!*file){
            throw std::runtime_error("Failed to write to file " + filename);
        }
    }

    void rewindForReading() {
        if (!isOpen()) {
            throw std::runtime_error("File is not open");
        }

        file->flush();
        file->clear();
        file->seekg(0, std::ios::beg);

        if (!*file) {
            throw std::runtime_error("Failed to seek in file");
        }
    }
    
    string readLine() {
        // Read and return one line from file
        if(file->is_open()){
            std::string line;
            if(std::getline(*file, line)){
                return line;
            }
        }
        return "";  // Replace with actual implementation
    }
    
    bool isOpen() const  noexcept{
        // YOUR CODE HERE
        // Check if file is open
        return file && file->is_open();
    }
};

// TODO 2: Implement RAII memory manager using smart pointers
template<typename T>
class SmartBuffer {
private:
    size_t size;
    std::unique_ptr<T[]> data;
    
public:
    explicit SmartBuffer(size_t size): size(size) {
        // Initialize smart pointer with array of given size
        // Store the size
        data = (size > 0) ? std::make_unique<T[]>(size): nullptr;
        std::cout << "Buffer allocated: " << size << " elements\n";
    }
    
    // TODO 3: Implement move semantics
    SmartBuffer(SmartBuffer&& other) noexcept: size(std::move(other.size)), data(std::move(other.data)) {}
    
    SmartBuffer& operator=(SmartBuffer&& other) noexcept {
        // Move assignment operator implementation
        // Handle self-assignment
        // Transfer ownership from other
        if(this != &other){
            data = std::move(other.data);
            size = std::move(other.size);
            other.size = 0;
            cout << "SmartBuffer moved (move assignment)" << endl;
        }
        return *this;
    }
    
    // Delete copy operations to enforce move-only semantics
    SmartBuffer(const SmartBuffer&) = delete;
    SmartBuffer& operator=(const SmartBuffer&) = delete;
    
    // Default destructor (smart pointer handles cleanup automatically)
    ~SmartBuffer() = default;
    
    T& operator[](size_t index) {
        // Return reference to element at index
        // Add bounds checking if desired
        if(index>= size){
            throw std::out_of_range("Index out of bounds");
        }
        return data[index];
    }
    
    const T& operator[](size_t index) const {
        // Const version of operator[]
        if(index >= size){
            throw  std::out_of_range("Index out of bounds");
        }
        return data[index];
    }
    
    size_t getSize() const {
        return size;  
    }
    
    T* get() {
        // Return raw pointer from smart pointer
        return data.get();  // Replace with actual implementation
    }
    
    const T* get() const noexcept {
        return data.get();
    }
};

// ============================================================================
// PART 2: Modern Design Patterns
// ============================================================================

// TODO 4: Implement modern Observer pattern using std::function
class EventPublisher {
private:
    // Use std::vector<std::function<void(const std::string&)>> for observers
    using Handler = std::function<void(const std::string&)>;
    std::vector<Handler> observers;

    
public:
    void subscribe(const std::function<void(const std::string&)>& observer) {
        // Add observer to collection
        if(!observer){
            throw std::invalid_argument("observer cannot be empty");
        }
        observers.push_back(observer);
    }
    
    void notify(const std::string& event) {
        // Call all observers with the event
        cout << "Publishing event: " << event << endl;
        for(auto& observer: observers ){
            observer(event);
        }
    }
};

// TODO 5: Implement modern Factory pattern with smart pointers
class Shape {
public:
    virtual ~Shape() = default;
    virtual void draw() const = 0;
    virtual double area() const = 0;
};

class Circle : public Shape {
private:
    double radius;
    
public:
    explicit Circle(double r) : radius(r) {}
    
    void draw() const override {
        cout << "Drawing Circle with radius " << radius << endl;
    }
    
    double area() const override {
        return 3.14159 * radius * radius;
    }
};

class Rectangle : public Shape {
private:
    double width, height;
    
public:
    Rectangle(double w, double h) : width(w), height(h) {}
    
    void draw() const override {
        cout << "Drawing Rectangle " << width << "x" << height << endl;
    }
    
    double area() const override {
        return width * height;
    }
};

class ShapeFactory {
public:
    enum class ShapeType { CIRCLE, RECTANGLE };
    
    static std::unique_ptr<Shape> createShape(ShapeType type, double param1, double param2 = 0) {
        // YOUR CODE HERE
        // Create appropriate shape based on type:
        // - CIRCLE: use param1 as radius
        // - RECTANGLE: use param1 as width, param2 as height
        // Return std::unique_ptr<Shape>
        switch (type) {
            case ShapeType::CIRCLE:
                return std::make_unique<Circle>(param1);
            case ShapeType::RECTANGLE:
                return std::make_unique<Rectangle>(param1, param2);
            default:
                throw std::invalid_argument("Unknown shape type");        
        }
    }
};

// ============================================================================
// PART 3: Modern C++ Features Integration
// ============================================================================

// TODO 6: Implement container with modern features
class ModernDataContainer {
private:
    std::vector<std::string> data;
    
public:
    // Constructor with initializer list
    ModernDataContainer(std::initializer_list<std::string> init) : data(init) {}
    
    // TODO 7: Add data using perfect forwarding
    template<typename T>
    void addData(T&& item) {
        // Use std::forward to perfectly forward the parameter
        // Add to data vector using emplace_back
        data.emplace_back(std::forward<T>(item));
    }
    
    // Range-based for loop support
    auto begin() { return data.begin(); }
    auto end() { return data.end(); }
    auto begin() const { return data.begin(); }
    auto end() const { return data.end(); }
    
    // Modern algorithm usage
    void processData(const std::function<void(const std::string&)>& processor) {
        // Use std::for_each with the processor function
        std::for_each(data.begin(), data.end(), processor);
    }
    
    //Implement modern search with auto and lambda
    auto findItem(const std::string& target) {
        // Use std::find to search for target
        // Return iterator (use auto for return type)
        return std::find(data.begin(), data.end(),target); 
    }
    
    // Modern filtering with lambda
    std::vector<std::string> filter(const std::function<bool(const std::string&)>& predicate) {
        std::vector<std::string> result;
        // Use std::copy_if with predicate to filter data
        // Copy matching items to result vector
        std::copy_if(data.begin(), data.end(), std::back_inserter(result), predicate);
        return result;
    }
    
    size_t size() const { return data.size(); }
};

// ============================================================================
// PART 4: Performance Measurement and Testing
// ============================================================================

class PerformanceTester {
public:
    //Implement performance timing
    template<typename Func>
    static double measureTime(Func&& function) {
        // YOUR CODE HERE
        // 1. Get start time using std::chrono::high_resolution_clock
        // 2. Call the function
        // 3. Get end time
        // 4. Calculate duration in milliseconds
        // 5. Return duration as double
        
        auto start = std::chrono::high_resolution_clock::now();
        std::forward<Func>(function)();  // Remove this line in your implementation
        auto end = std::chrono::high_resolution_clock::now();
        return std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count(); 
    }
    
    static void testMoveSemantics() {
        cout << "\n=== Testing Move Semantics ===" << endl;
        
        const size_t BUFFER_SIZE = 1000000;
        
        // Test move vs copy performance
        auto createBuffer = []() {
            return SmartBuffer<int>(BUFFER_SIZE);
        };
        
        // TODO 10: Measure move operation performance
        double moveTime = measureTime([&]() {
            SmartBuffer<int> buffer = createBuffer();  // Should use move
            // Use buffer somehow to prevent optimization
            volatile int sum = buffer[0] + buffer[BUFFER_SIZE-1];
            (void)sum;  // Suppress unused variable warning
        });
        
        cout << "Move operation time: " << moveTime << " ms" << endl;
    }
};

// Test functions
void testRAII() {
    cout << "\n=== Testing RAII ===" << endl;
    
    try {
        // Test file handling with RAII
        FileHandler writer("test_output.txt");
        writer.writeLine("Hello, RAII!");
        writer.writeLine("Automatic cleanup on scope exit");
        cout << "✓ File operations completed successfully" << endl;
    } catch (const exception& e) {
        cout << "File error: " << e.what() << endl;
    }
    
    // File should be automatically closed here due to RAII
    
    // Test smart buffer with RAII
    {
        SmartBuffer<int> buffer(100);
        for (size_t i = 0; i < buffer.getSize(); ++i) {
            buffer[i] = static_cast<int>(i * i);
        }
        cout << "✓ Smart buffer created and used, size: " << buffer.getSize() << endl;
    }
    // Buffer should be automatically cleaned up here
    
    cout << "✓ All resources automatically cleaned up via RAII" << endl;
}

void testMoveSemantics() {
    cout << "\n=== Testing Move Semantics ===" << endl;
    
    // Create buffer and move it
    SmartBuffer<int> buffer1(50);
    buffer1[0] = 42;
    buffer1[49] = 99;
    
    cout << "Buffer1 size before move: " << buffer1.getSize() << endl;
    cout << "Buffer1[0] = " << buffer1[0] << ", Buffer1[49] = " << buffer1[49] << endl;
    
    // Move buffer1 to buffer2
    SmartBuffer<int> buffer2 = std::move(buffer1);
    
    cout << "After move:" << endl;
    cout << "Buffer2 size: " << buffer2.getSize() << endl;
    cout << "Buffer2[0] = " << buffer2[0] << ", Buffer2[49] = " << buffer2[49] << endl;
    cout << "✓ Move semantics working correctly" << endl;
}

void testModernPatterns() {
    cout << "\n=== Testing Modern Design Patterns ===" << endl;
    
    // Test Observer pattern
    EventPublisher publisher;
    
    publisher.subscribe([](const string& event) {
        cout << "Observer 1 received: " << event << endl;
    });
    
    publisher.subscribe([](const string& event) {
        cout << "Observer 2 logged: " << event << endl;
    });
    
    publisher.notify("System Started");
    publisher.notify("User Logged In");
    
    // Test Factory pattern
    auto circle = ShapeFactory::createShape(ShapeFactory::ShapeType::CIRCLE, 5.0);
    auto rectangle = ShapeFactory::createShape(ShapeFactory::ShapeType::RECTANGLE, 4.0, 6.0);
    
    if (circle && rectangle) {
        circle->draw();
        cout << "Circle area: " << circle->area() << endl;
        
        rectangle->draw();
        cout << "Rectangle area: " << rectangle->area() << endl;
        
        cout << "✓ Factory pattern working correctly" << endl;
    }
}

void testModernFeatures() {
    cout << "\n=== Testing Modern C++ Features ===" << endl;
    
    // Test container with modern features
    ModernDataContainer container{"Apple", "Banana", "Cherry"};
    
    // Add data using perfect forwarding
    string fruit = "Date";
    container.addData(fruit);        // lvalue
    container.addData("Elderberry"); // rvalue
    
    cout << "Container contents:" << endl;
    for (const auto& item : container) {
        cout << "- " << item << endl;
    }
    
    // Test modern algorithm usage
    cout << "\nProcessing data with lambda:" << endl;
    container.processData([](const string& item) {
        cout << "Processing: " << item << endl;
    });
    
    // Test search functionality
    auto found = container.findItem("Banana");
    if (found != container.end()) {
        cout << "✓ Found item: " << *found << endl;
    }
    
    // Test filtering
    auto longNames = container.filter([](const string& item) {
        return item.length() > 5;
    });
    
    cout << "Items with more than 5 characters:" << endl;
    for (const auto& item : longNames) {
        cout << "- " << item << endl;
    }
    
    cout << "✓ Modern features working correctly" << endl;
}

int main() {
    cout << "=== Modern C++ Idioms and Patterns Lab ===" << endl;
    cout << "Learning RAII principles, move semantics, and modern design patterns" << endl;
    
    testRAII();
    testMoveSemantics();
    testModernPatterns();
    testModernFeatures();
    
    // Performance testing
    PerformanceTester::testMoveSemantics();
    
    cout << "\n=== Lab Complete! ===" << endl;
    cout << "You've successfully implemented modern C++ idioms and patterns!" << endl;
    
    return 0;
}

/*
REQUIREMENTS CHECKLIST:
□ Implement RAII file handler with automatic resource cleanup
□ Create smart buffer using std::unique_ptr with move semantics
□ Implement move constructor and move assignment operator correctly
□ Create Observer pattern using std::function and lambda expressions
□ Implement Factory pattern returning std::unique_ptr<Shape>
□ Use perfect forwarding in template functions
□ Apply modern C++ features (auto, range-based loops, algorithms)
□ Demonstrate performance benefits of move semantics
□ Ensure automatic resource cleanup in all RAII implementations

COMMON MISTAKES TO AVOID:
- Forgetting to check if file operations succeed
- Not properly implementing move semantics (leaving moved-from object in valid state)
- Memory leaks when not using RAII properly
- Incorrect perfect forwarding implementation
- Not handling self-assignment in move assignment operator

VALIDATION:
- File operations should work without memory leaks
- Move operations should transfer ownership correctly
- Observer pattern should notify all subscribers
- Factory should create correct shape types
- Modern features should provide clean, expressive code
- Performance testing should show benefits of move semantics
*/