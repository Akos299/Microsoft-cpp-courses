/*
 * File: sensor_processor.cpp
 * Purpose: This program implements a sensor data processing system
 *          demonstrating STL container selection for different data
 *          access patterns including fast lookups and priority processing.
 */

#include <iostream>
#include <vector>
#include <unordered_map>
#include <map>
#include <queue>
#include <string>
#include <iomanip>

using namespace std;

struct SensorReading {
    int sensorId;
    string location;
    string sensorType;
    double value;
    
    SensorReading(int id, string loc, string type, double val)
        : sensorId(id), location(loc), sensorType(type), value(val) {}
SensorReading() = default;
};

struct Alert {
    int priority;
    string message;
    string alertType;
    
    Alert(int p, string msg, string type)
        : priority(p), message(msg), alertType(type) {}
};

struct AlertCompare{
    bool operator()(Alert& a, Alert& b){
        return  a.priority < b.priority;
    }
};

class SensorProcessor {
private:
    // TODO: Choose appropriate containers for different access patterns
    // Hint: You'll need containers for ID lookup, location grouping, and priority processing
    unordered_map<int, SensorReading> sensorData; // for ID lookup
    unordered_map<string, vector<int>> locationIndex; // for location grouping
    priority_queue<Alert, vector<Alert>,  AlertCompare> alertQueue;
    
public:
    /**
     * GRADED CHALLENGE 1
     * TASK: Implement addSensorReading to store sensor data with fast ID lookup 
     * and location-based grouping capabilities.
     */
    void addSensorReading(const SensorReading& reading) {
        sensorData[reading.sensorId] = reading;
        locationIndex[reading.location].push_back(reading.sensorId);
        cout << "Added sensor " << reading.sensorId << " (" << reading.sensorType 
             << ") to " << reading.location << endl;
    }
    
    /**
     * GRADED CHALLENGE 2  
     * TASK: Implement addAlert to store alerts in priority order.
     * Higher priority numbers should be processed first.
     */
    void addAlert(const Alert& alert) {
        alertQueue.push(alert);
        cout << "Added alert: Priority " << alert.priority << " - " 
             << alert.message << endl;
    }
    
    /**
     * GRADED CHALLENGE 3
     * TASK: Implement processNextAlert to handle the highest priority alert.
     * Display the alert information and remove it from the queue.
     */
    void processNextAlert() {
        // Handle empty queue case
        if (alertQueue.empty()) {
            cout << "No alerts to process - queue is empty" << endl;
            return;
        }
        Alert highAlert = alertQueue.top();
        cout << "Processing Priority ["<< highAlert.priority<<"]: "<<"["<<highAlert.message<<"] (["<<highAlert.alertType<<"])"<<endl;
        alertQueue.pop();
    }
    
    /**
     * GRADED CHALLENGE 4
     * TASK: Implement findSensorById for fast sensor lookup by ID.
     * Return pointer to sensor data or nullptr if not found.
     */
    SensorReading* findSensorById(int sensorId) {
        // YOUR CODE HERE
        // Achieve O(1) average case lookup performance
        // Return nullptr if sensor not found
        auto it  = sensorData.find(sensorId);
        if(it != sensorData.end()){
            return &(it->second);
        }
        return nullptr;

    }
    
    /**
     * GRADED CHALLENGE 5
     * TASK: Implement getSensorsByLocation to return all sensors
     * in a specified building location.
     */
    vector<SensorReading*> getSensorsByLocation(const string& location) {
        // YOUR CODE HERE
        // Return all sensors at the specified location
        // Return empty vector if location not found
         vector<SensorReading*> result;
         auto it = locationIndex.find(location);
        if(it!=locationIndex.end()){
            for(int sensorId: it->second){
                auto findRes = findSensorById(sensorId);
                if (!findRes) result.push_back(findRes);
                auto sensorIt = sensorData.find(sensorId);
            }
        }

        return result;
    }

    /**
     * Function: runTests
     * Description: Test harness to verify basic functionality
     */
    void runTests() {
        cout << "=== Sensor Processing System Tests ===" << endl;
        
        // Add test sensor data
        cout << "Adding sensor readings..." << endl;
        addSensorReading(SensorReading(101, "Building_A", "temperature", 72.5));
        addSensorReading(SensorReading(102, "Building_A", "humidity", 45.2));
        addSensorReading(SensorReading(201, "Building_B", "temperature", 68.1));
        addSensorReading(SensorReading(202, "Building_B", "humidity", 52.7));
        addSensorReading(SensorReading(301, "Building_C", "temperature", 74.3));
        cout << "✓ Sensor data added" << endl;
        
        // Test ID lookup
        cout << "\nTesting ID-based lookup..." << endl;
        auto sensor = findSensorById(102);
        if (sensor != nullptr) {
            cout << "✓ Found sensor " << sensor->sensorId << " in " << sensor->location << endl;
        }
        
        // Test location queries
        cout << "\nTesting location-based queries..." << endl;
        auto buildingASensors = getSensorsByLocation("Building_A");
        cout << "✓ Found " << buildingASensors.size() << " sensors in Building_A" << endl;
        
        // Test priority processing
        cout << "\nTesting priority alert processing..." << endl;
        addAlert(Alert(3, "Temperature threshold exceeded", "WARNING"));
        addAlert(Alert(9, "Critical system failure", "CRITICAL"));
        addAlert(Alert(1, "Low battery detected", "INFO"));
        addAlert(Alert(7, "Network connectivity issue", "ERROR"));
        
        cout << "Processing alerts by priority:" << endl;
        processNextAlert();
        processNextAlert();
        processNextAlert();
        processNextAlert();
        
        cout << "\n=== Testing Complete ===" << endl;
    }
};

/**
 * Function: main
 * Description: Program entry point demonstrating the sensor processing system
 */
int main() {
    cout << "=== Sensor Data Processing System ===" << endl;
    cout << "Demonstrating STL container selection for efficient data processing\n" << endl;
    
    SensorProcessor processor;
    processor.runTests();
    
    return 0;
}
