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

int main() {
    std::string logFile = "collapse_timestamp_test.log";
    std::ofstream(logFile, std::ios::trunc).close();

    L::initialize(logFile);
    L::enableEchoMode(false);
    // Enable timestamps (default is DateTime)
    L::setTimestampMode(L::TimestampMode::EpochMilliseconds); 
    L::setBatchInterval(std::chrono::milliseconds(2000));
    L::setCollapseRepeatedMessages(true);

    std::cout << "Logging repeated messages with timestamps..." << std::endl;
    L().info() << "Repeated Message";
    std::this_thread::sleep_for(std::chrono::milliseconds(10)); // Ensure timestamp diff
    L().info() << "Repeated Message";
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    L().info() << "Repeated Message";

    L::flush();

    // Check if collapsed
    if (!fileContains(logFile, "Repeated Message (x3)")) {
        std::cerr << "File should contain 'Repeated Message (x3)' even with different timestamps!" << std::endl;
        
        // Print file content for debugging
        std::ifstream file(logFile);
        std::cout << "File content:\n" << file.rdbuf() << std::endl;
        
        return 1;
    }

    std::cout << "Test passed!" << std::endl;
    return 0;
}
