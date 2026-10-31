/**
* This contain of this file is inspired by the post of Alexander Obregon
* at https://medium.com/@AlexanderObregon/understanding-c-coroutine-implementation-8e6e5a2c3edd
*
*/

#include <coroutine>
#include <exception>
#include <fstream>
#include <iostream>
#include <string>
#include <thread>
#include <chrono>
#include <utility>

//1. FileReader — Coroutine return type
// Manages the coroutine's lifetime and provides access to its execution state through std::coroutine_handle.
// The return type must provide a nested promise_type. This is the interface between the coroutine and 
// the object returned to the caller.

struct FileReader {

    struct promise_type{
      FileReader  std::coroutine_handle<promise_type>::from_promise(*this);

      std::suspend_always initial_suspend() noexcept {return {}; }
      std::suspend_always final_suspend() noexcept {return {}; }
      void return_void() {}
      void unhandled_exception() {std::terminate();}
    };

    std::coroutine_handle<promise_type> h;
    explicit FileReader(std::coroutine_handle<promise_type> h): h(h) {}
    FileReader(FileReader&& other) noexcept: h(std::exchange(other.h, {})){}
    FileReader(const FileReader&) = delete;
    ~FileReader() { if (h) h.destroy(); }
};


// In C++20, co_await is the mechanism that allows a coroutine to suspend its execution and potentially resume later

struct Awaiter {
    std::ifstream file;
    std::string line;
    bool finished = false;

    /** await_ready()
        Is the operation already complete?
        - true: don't suspend; proceed to await_resume().
        - false: proceed to await_suspend().
    */
    bool await_ready() const { return finished; }

    /** await_suspend(handle)
        The coroutine is suspended. This function decides what happens next.
    */
    void await_suspend(std::coroutine_handle<> h) {
        if (std::getline(file, line)) {
            std::cout << "Processing: " << line << '\n';
        } else {
            finished = true;
        }
        h.resume();
    }

    /** wait_resume()
        The coroutine resumes. This function retrieves the result or propagates an exception.
    */
    void await_resume() {}
};


struct LineAwaiter {
    std::ifstream& file;
    std::string& line;
    bool finished = false;

    bool await_ready() const noexcept {
        return false;
    }

    bool await_suspend(std::coroutine_handle<>) noexcept {
        return false;
    }

    bool await_resume() {
        if (std::getline(file, line)) {
            std::cout << "Processing: " << line << '\n';
            return true;
        }

        finished = true;
        return false;
    }
};


// awaiter object
Awaiter readAsync(const std::string& filename) {
    Awaiter a;
    a.file.open(filename);
    return a;
}

FileReader processFiles() {
    auto r1 = readAsync("file1.txt");
    auto r2 = readAsync("file2.txt");

    while (!r1.finished || !r2.finished) {
        co_await r1;
        co_await r2;
    }
    co_return;
}
int main(){
    FileReader fr = processFiles();
    fr.h.resume();
    return 0;
}