#include <iostream>
#include <memory>
#include <string>
#include <fstream>
#include <stdexcept>
#include <chrono>
#include <thread>

// Simulate database connection resource
struct DatabaseConnection {
    std::string connectionString;
    bool connected;
    
    DatabaseConnection(const std::string& connStr) : connectionString(connStr), connected(false) {
        // Simulate connection establishment
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        connected = true;
        std::cout << "Database connected: " << connectionString << std::endl;
    }
    
    ~DatabaseConnection() noexcept{
        if (connected) {
            std::cout << "Database disconnected: " << connectionString << std::endl;
            connected = false;
        }
    }
    
    void execute(const std::string& query) {
        if (!connected) throw std::runtime_error("Not connected");
        std::cout << "Executing: " << query << std::endl;
        if (query == "ERROR") {
            throw std::runtime_error("Simulated query failure");
        }
    }
};

class DatabaseManager {
private:
    // Add RAII implementation
    std::unique_ptr<DatabaseConnection> connection;
    
    
public:
    DatabaseManager(const std::string& connectionString):
    connection(std::make_unique<DatabaseConnection>(connectionString)) {
        // Initialize database connection using RAII principles
        // Ensure connection is established or throw exception
        if(!connection->connected){
            throw std::runtime_error("Failed to establish database connection");
        }
    }

    DatabaseManager(const DatabaseManager&) = delete;
    DatabaseManager& operator=(const DatabaseManager&) = delete;

    // Transfer ownership safely
    DatabaseManager(DatabaseManager&&) noexcept = default;
    DatabaseManager& operator=(DatabaseManager&&) noexcept = default;
    
    ~DatabaseManager() =default;
    
    void executeQuery(const std::string& query) {
        // Execute query with proper error handling
        // Should work even if exceptions occur
        if(!isConnected()){
            throw std::runtime_error("Cannot execute query: no acive connection");
        }
        connection->execute(query);
    }
    
    bool isConnected() const {
        // Check connection status
        return connection && connection->connected;
    }
};



int main() {
    // Test 1: Successful operation
    try {
        std::cout << "\n--- Test 1: Success ---\n";

        DatabaseManager db("localhost:5432/mydb");

        std::cout << "Connected? "
                  << std::boolalpha << db.isConnected() << '\n';

        db.executeQuery("SELECT * FROM users");

    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << '\n';
    }

    // Test 2: Query throws an exception
    try {
        std::cout << "\n--- Test 2: Query failure ---\n";

        DatabaseManager db("localhost:5432/mydb");
        db.executeQuery("ERROR");

    } catch (const std::exception& e) {
        std::cerr << "Caught exception: " << e.what() << '\n';
    }

    // Test 3: Exception after acquiring a connection
    try {
        std::cout << "\n--- Test 3: Exception during operation ---\n";

        DatabaseManager db("localhost:5432/mydb");
        throw std::runtime_error("Unexpected application failure");

    } catch (const std::exception& e) {
        std::cerr << "Caught exception: " << e.what() << '\n';
    }

    // Test 4: Ownership transfer
    std::cout << "\n--- Test 4: Move ownership ---\n";
    {
        DatabaseManager first("localhost:5432/anotherdb");
        DatabaseManager second(std::move(first));

        std::cout << "First connected? "
                  << first.isConnected() << '\n';
        std::cout << "Second connected? "
                  << second.isConnected() << '\n';

        second.executeQuery("SELECT 1");
    }

    return 0;
}