
#include <fstream>
#include <vector>
#include <string>
#include <sstream>
#include <random>
#include <cstddef>
#include <cstdlib>
#include <iostream>
#include <cstdint>
#include <stdexcept>
#include <limits>
#include <iterator>
#include <algorithm>
#include <iostream>
#include <iomanip>

class PrecisionTimer {
private:
    // TODO: Add timing infrastructure
    using Clock = std::chrono::high_resolution_clock;
    Clock::time_point startTime ;
    
public:
    PrecisionTimer(): startTime (Clock::now()) {
        // Initialize timing system
    }
    
    void startTiming() {
        // Record start time using high_resolution_clock
        startTime = Clock::now();
    }
    
    double getElapsedMicroseconds() {
        // Calculate elapsed time in microseconds
        const Clock::time_point endTime = Clock::now();
        return std::chrono::duration_cast<std::chrono::microseconds>(endTime-startTime).count();
    }
    
    double getElapsedMilliseconds() {
        // Calculate elapsed time in milliseconds
        const Clock::time_point endTime = Clock::now();
        return std::chrono::duration_cast<std::chrono::milliseconds>(endTime-startTime).count();
    }
};

class StatisticalBenchmark {
private:
    std::vector<double> measurements;
    std::string benchmarkName;

    // Approximate 95% Student-t critical values for df = 1..30.
    // For larger degrees of freedom, use the normal approximation.
    static double tCritical95(std::size_t degreesOfFreedom) {
        static const double values[] = {
            0.0,
            12.706, 4.303, 3.182, 2.776, 2.571,
            2.447, 2.365, 2.306, 2.262, 2.228,
            2.201, 2.179, 2.160, 2.145, 2.131,
            2.120, 2.110, 2.101, 2.093, 2.086,
            2.080, 2.074, 2.069, 2.064, 2.060,
            2.056, 2.052, 2.048, 2.045, 2.042
        };

        if (degreesOfFreedom == 0)
            return std::numeric_limits<double>::quiet_NaN();

        if (degreesOfFreedom <= 30)
            return values[degreesOfFreedom];

        return 1.96;
    }
    
public:
    StatisticalBenchmark(const std::string& name) : benchmarkName(name) {}
    
    template<typename Func>
    void runBenchmark(Func function, int trials = 10) {
        // Implement comprehensive benchmarking
        // 1. Clear previous measurements
        // 2. Perform warm-up run
        // 3. Execute function 'trials' times
        // 4. Record timing for each execution
        // 5. Store measurements for statistical analysis
        if (trials < 2) {
            throw std::invalid_argument(
                "At least 2 trials are required for statistical analysis."
            );
        }
        measurements.clear();
        measurements.reserve(static_cast<std::size_t>(trials));
        function();
       
        double elapseTime = 0.0;
        for(size_t i = 0; i < trials; i++){

            PrecisionTimer timerObject;
            timerObject.startTiming();
            function();
            elapseTime = timerObject.getElapsedMicroseconds();
            measurements.push_back(elapseTime);
        }
    }


    template<typename Func, typename Setup>
    void runBenchmark(Func function, Setup setup, int trials = 10) {
        if (trials < 2) {
            throw std::invalid_argument(
                "At least 2 trials are required."
            );
        }

        measurements.clear();
        measurements.reserve(static_cast<std::size_t>(trials));

        // Warm-up: setup and execution are excluded from measurements.
        setup();
        function();

        for (int i = 0; i < trials; ++i) {
            setup(); // Prepare data before timing.

            PrecisionTimer timerObject;
            timerObject.startTiming();

            function();

            measurements.push_back(
                timerObject.getElapsedMicroseconds()
            );
        }
    }

    
    void printStatistics() {
        // TODO: Calculate and display statistical analysis
        // - Average execution time
        // - Minimum and maximum times
        // - Standard deviation
        // - 95% confidence interval
        // - Coefficient of variation
        if(measurements.empty()) return;
        double mean = std::accumulate(measurements.begin(), measurements.end(), 0.0) /  measurements.size();
        auto [min_it, max_it] = std::minmax_element(measurements.begin(), measurements.end());
        double min = (min_it != measurements.end()) ? *min_it : 0.0;
        double max = (max_it != measurements.end()) ? *max_it : 0.0;
        double variance = 0.0;
        for(auto& m:measurements){
            variance += (m-mean)*(m-mean);
        }
        double stddev = std::sqrt(variance / (measurements.size() - 1));

        double cv = (mean != 0.0) ? (stddev / std::abs(mean)) * 100.0 : 0.0;

        const double standardError =
            stddev / std::sqrt(static_cast<double>(measurements.size()));
        const double criticalValue = tCritical95(measurements.size() - 1);
        const double marginOfError = criticalValue * standardError;

        std::cout << "\n=== " << benchmarkName << " ===\n";
        std::cout << std::fixed << std::setprecision(4);

        std::cout << "Trials:                 " << measurements.size() << '\n';
        std::cout << "Mean:                   " << mean << " us\n";
        std::cout << "Minimum:                " << min << " us\n";
        std::cout << "Maximum:                " << max << " us\n";
        std::cout << "Sample std. deviation:  "  << stddev << " us\n";
        std::cout << "Standard error:         " << standardError << " us\n";

        std::cout << "95% CI for mean:        ["
                  << mean - marginOfError << ", "
                  << mean + marginOfError << "] us\n";

        std::cout << "Coefficient of variation: " << cv << "%\n";
    }
    
    double getAverageTime() const {
        // Return average execution time
        if (measurements.empty())
            return 0.0;

        return std::accumulate(
            measurements.begin(),
            measurements.end(),
            0.0
        ) / measurements.size();
    }
    
    double getStandardDeviation() const {
        // Calculate standard deviation of measurements
        const std::size_t n = measurements.size();

        if (n < 2)
            return 0.0;

        const double mean = getAverageTime();
        double sumSquaredDifferences = 0.0;

        for (double value : measurements) {
            const double difference = value - mean;
            sumSquaredDifferences += difference * difference;
        }

        // Sample standard deviation (Bessel's correction).
        return std::sqrt(
            sumSquaredDifferences / static_cast<double>(n - 1)
        );
    }
};


class IOPerformanceAnalyzer {
public:
    // Frequent flushes: intentionally inefficient text output.
    static void writeDataUnbuffered(
        const std::string& filename,
        const std::vector<int>& data
    ) {
        std::ofstream out(filename);

        if (!out)
            throw std::runtime_error("Cannot open file for writing: " + filename);

        for (int value : data) {
            out << value << '\n';
            out.flush();
            if (!out)
                throw std::runtime_error("Write failed: " + filename);
        }

        out.close();
        if (!out)
            throw std::runtime_error("File close failed: " + filename);
    }

    // Format the text in memory, then write it in one large operation.
    static void writeDataBuffered(
        const std::string& filename,
        const std::vector<int>& data
    ) {
        std::ostringstream buffer;

        for (int value : data)
            buffer << value << '\n';

        const std::string contents = buffer.str();

        std::ofstream out(filename, std::ios::binary);
        if (!out)
            throw std::runtime_error("Cannot open file for writing: " + filename);

        out.write(contents.data(),
                  static_cast<std::streamsize>(contents.size()));

        out.close();
        if (!out)
            throw std::runtime_error("Write failed: " + filename);
    }

    // Bulk binary output: compact and avoids integer-to-text conversion.
    static void writeDataBulk(
        const std::string& filename,
        const std::vector<int>& data
    ) {
        std::ofstream out(
            filename,
            std::ios::binary | std::ios::trunc
        );

        if (!out)
            throw std::runtime_error("Cannot open file for writing: " + filename);

        if (!data.empty()) {
            out.write(
                reinterpret_cast<const char*>(data.data()),
                static_cast<std::streamsize>(
                    data.size() * sizeof(int)
                )
            );
        }

        out.close();
        if (!out)
            throw std::runtime_error("Binary write failed: " + filename);
    }

    // Read text integers one at a time through stream extraction.
    static std::vector<int> readDataUnbuffered(
        const std::string& filename
    ) {
        std::ifstream in(filename);
        if (!in)
            throw std::runtime_error("Cannot open file for reading: " + filename);

        std::vector<int> data;
        int value;

        while (in >> value)
            data.push_back(value);

        if (!in.eof())
            throw std::runtime_error("Text read/parse failed: " + filename);

        return data;
    }

    // Load the complete text file, then parse it from memory.
    static std::vector<int> readDataBuffered(
        const std::string& filename
    ) {
        std::ifstream in(filename, std::ios::binary);
        if (!in)
            throw std::runtime_error("Cannot open file for reading: " + filename);

        const std::string contents(
            (std::istreambuf_iterator<char>(in)),
            std::istreambuf_iterator<char>()
        );

        if (in.bad())
            throw std::runtime_error("File read failed: " + filename);

        std::istringstream parser(contents);
        std::vector<int> data;
        int value;

        while (parser >> value)
            data.push_back(value);

        if (!parser.eof())
            throw std::runtime_error("Text parse failed: " + filename);

        return data;
    }

    // Bulk binary input, paired with writeDataBulk().
    static std::vector<int> readDataBulk(
        const std::string& filename
    ) {
        std::ifstream in(filename, std::ios::binary | std::ios::ate);
        if (!in)
            throw std::runtime_error("Cannot open binary file: " + filename);

        const std::streampos end = in.tellg();
        if (end < 0)
            throw std::runtime_error("Cannot determine file size: " + filename);

        const auto bytes = static_cast<std::uintmax_t>(end);

        if (bytes % sizeof(int) != 0)
            throw std::runtime_error("Invalid binary file size: " + filename);

        const auto count = bytes / sizeof(int);

        if (count > std::numeric_limits<std::size_t>::max())
            throw std::runtime_error("File too large: " + filename);

        std::vector<int> data(static_cast<std::size_t>(count));

        in.seekg(0, std::ios::beg);

        if (!data.empty()) {
            in.read(
                reinterpret_cast<char*>(data.data()),
                static_cast<std::streamsize>(bytes)
            );
        }

        if (!in)
            throw std::runtime_error("Binary read failed: " + filename);

        return data;
    }

    static std::size_t getFileSize(const std::string& filename) {
        std::ifstream in(filename, std::ios::binary | std::ios::ate);

        if (!in)
            throw std::runtime_error("Cannot open file: " + filename);

        const std::streampos end = in.tellg();
        if (end < 0)
            throw std::runtime_error("Cannot determine file size: " + filename);

        return static_cast<std::size_t>(end);
    }

    static std::vector<int> generateTestData(std::size_t count) {
        std::vector<int> data(count);
        std::mt19937 rng(12345);
        std::uniform_int_distribution<int> distribution(-1'000'000, 1'000'000);

        for (int& value : data)
            value = distribution(rng);

        return data;
    }
};





volatile std::uint64_t ioChecksum = 0;

int main() {
    const std::vector<std::size_t> sizes = {
        1'000, 10'000, 100'000
    };

    constexpr int writeTrials = 5;
    constexpr int readTrials = 10;

    for (const std::size_t n : sizes) {
        const std::vector<int> data =
            IOPerformanceAnalyzer::generateTestData(n);

        const std::string prefix = "io_test_" + std::to_string(n);

        const std::string unbufferedFile = prefix + "_flush.txt";
        const std::string bufferedFile   = prefix + "_buffered.txt";
        const std::string bulkFile       = prefix + "_bulk.bin";

        std::cout << "\n========================================\n"
                  << "Elements: " << n << '\n'
                  << "========================================\n";

        // ---------- WRITE BENCHMARKS ----------

        StatisticalBenchmark unbufferedWrite(
            "Write: flush after every integer"
        );

        unbufferedWrite.runBenchmark([&]() {
            IOPerformanceAnalyzer::writeDataUnbuffered(
                unbufferedFile, data
            );
        }, writeTrials);

        unbufferedWrite.printStatistics();

        StatisticalBenchmark bufferedWrite(
            "Write: buffered text"
        );

        bufferedWrite.runBenchmark([&]() {
            IOPerformanceAnalyzer::writeDataBuffered(
                bufferedFile, data
            );
        }, writeTrials);

        bufferedWrite.printStatistics();

        StatisticalBenchmark bulkWrite(
            "Write: bulk binary"
        );

        bulkWrite.runBenchmark([&]() {
            IOPerformanceAnalyzer::writeDataBulk(
                bulkFile, data
            );
        }, writeTrials);

        bulkWrite.printStatistics();

        // File sizes are collected outside timed regions.
        const auto textFlushSize =
            IOPerformanceAnalyzer::getFileSize(unbufferedFile);
        const auto textBufferedSize =
            IOPerformanceAnalyzer::getFileSize(bufferedFile);
        const auto binarySize =
            IOPerformanceAnalyzer::getFileSize(bulkFile);

        std::cout << "\nFile sizes:\n"
                  << "Flush text:    " << textFlushSize << " bytes\n"
                  << "Buffered text: " << textBufferedSize << " bytes\n"
                  << "Bulk binary:   " << binarySize << " bytes\n";

        // ---------- READ BENCHMARKS ----------

        StatisticalBenchmark unbufferedRead(
            "Read: stream integer extraction"
        );

        unbufferedRead.runBenchmark([&]() {
            const auto result =
                IOPerformanceAnalyzer::readDataUnbuffered(
                    unbufferedFile
                );

            ioChecksum = result.size();
            if (!result.empty())
                ioChecksum += static_cast<std::uint64_t>(
                    static_cast<std::int64_t>(result.front())
                );
        }, readTrials);

        unbufferedRead.printStatistics();

        StatisticalBenchmark bufferedRead(
            "Read: load text then parse"
        );

        bufferedRead.runBenchmark([&]() {
            const auto result =
                IOPerformanceAnalyzer::readDataBuffered(
                    bufferedFile
                );

            ioChecksum = result.size();
            if (!result.empty())
                ioChecksum += static_cast<std::uint64_t>(
                    static_cast<std::int64_t>(result.back())
                );
        }, readTrials);

        bufferedRead.printStatistics();

        StatisticalBenchmark bulkRead(
            "Read: bulk binary"
        );

        bulkRead.runBenchmark([&]() {
            const auto result =
                IOPerformanceAnalyzer::readDataBulk(bulkFile);

            ioChecksum = result.size();
            if (!result.empty())
                ioChecksum += static_cast<std::uint64_t>(
                    static_cast<std::int64_t>(result.front())
                );
        }, readTrials);

        bulkRead.printStatistics();

        // Correctness checks outside the timed measurements.
        const auto textData =
            IOPerformanceAnalyzer::readDataBuffered(bufferedFile);
        const auto binaryData =
            IOPerformanceAnalyzer::readDataBulk(bulkFile);

        if (textData != data || binaryData != data) {
            throw std::runtime_error(
                "I/O validation failed for dataset size " +
                std::to_string(n)
            );
        }

        std::cout << "\nValidation: passed\n";
    }

    std::cout << "\nChecksum: " << ioChecksum << '\n';
    return 0;
}
