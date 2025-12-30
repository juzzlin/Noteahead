// MIT License
//
// Copyright (c) 2025 Jussi Lind <jussi.lind@iki.fi>
//
// https://github.com/juzzlin/SimpleLogger
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

#include "../../simple_logger.hpp"
#include <cassert>
#include <chrono>
#include <fstream>
#include <iostream>
#include <thread>
#include <vector>

using namespace juzzlin;

bool fileContains(const std::string& path, const std::string& content) {
    std::ifstream file(path);
    if (!file.is_open()) return false;
    std::string line;
    while (std::getline(file, line)) {
        if (line.find(content) != std::string::npos) return true;
    }
    return false;
}

bool fileIsEmpty(const std::string& path) {
    std::ifstream file(path);
    return file.peek() == std::ifstream::traits_type::eof();
}

int main() {
    std::string logFile = "batch_test.log";
    // Clear file
    std::ofstream(logFile, std::ios::trunc).close();

    L::initialize(logFile);
    L::enableEchoMode(false); // Disable echo to focus on file
    L::setTimestampMode(L::TimestampMode::None); // Simplify check

    // Set batch interval to 2 seconds
    L::setBatchInterval(std::chrono::milliseconds(2000));

    std::cout << "Logging Message 1..." << std::endl;
    L().info() << "Message 1";

    // Should be buffered
    if (!fileIsEmpty(logFile)) {
        std::cerr << "File should be empty after first log!" << std::endl;
        return 1;
    }

    std::cout << "Waiting 2.5 seconds..." << std::endl;
    std::this_thread::sleep_for(std::chrono::milliseconds(2500));

    std::cout << "Logging Message 2..." << std::endl;
    L().info() << "Message 2";

    // Should be flushed now (both messages)
    if (!fileContains(logFile, "Message 1")) {
        std::cerr << "File should contain Message 1!" << std::endl;
        return 1;
    }
    if (!fileContains(logFile, "Message 2")) {
        std::cerr << "File should contain Message 2!" << std::endl;
        return 1;
    }

    // Test explicit flush
    std::cout << "Testing explicit flush..." << std::endl;
    L().info() << "Message 3";
    if (fileContains(logFile, "Message 3")) {
        std::cerr << "Message 3 should be buffered!" << std::endl;
        return 1;
    }
    L::flush();
    if (!fileContains(logFile, "Message 3")) {
        std::cerr << "Message 3 should be flushed after L::flush()!" << std::endl;
        return 1;
    }

    // Test setBatchInterval(0) flushing
    std::cout << "Testing setBatchInterval(0) flushing..." << std::endl;
    L().info() << "Message 4";
    if (fileContains(logFile, "Message 4")) {
        std::cerr << "Message 4 should be buffered!" << std::endl;
        return 1;
    }
    L::setBatchInterval(std::chrono::milliseconds(0));
    if (!fileContains(logFile, "Message 4")) {
        std::cerr << "Message 4 should be flushed after L::setBatchInterval(0)!" << std::endl;
        return 1;
    }

    std::cout << "Test passed!" << std::endl;
    return 0;
}
