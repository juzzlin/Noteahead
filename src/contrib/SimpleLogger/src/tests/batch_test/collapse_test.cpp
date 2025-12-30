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
    std::string logFile = "collapse_test.log";
    std::ofstream(logFile, std::ios::trunc).close();

    L::initialize(logFile);
    L::enableEchoMode(false);
    L::setTimestampMode(L::TimestampMode::None); // Disable timestamps for easier matching
    L::setBatchInterval(std::chrono::milliseconds(1000));
    L::setCollapseRepeatedMessages(true);

    std::cout << "Logging repeated messages..." << std::endl;
    L().info() << "Repeated Message";
    L().info() << "Repeated Message";
    L().info() << "Repeated Message";

    L::flush();

    if (!fileContains(logFile, "Repeated Message (x3)")) {
        std::cerr << "File should contain 'Repeated Message (x3)'!" << std::endl;
        return 1;
    }

    // Test non-repeated
    L().info() << "Unique Message 1";
    L().info() << "Unique Message 2";
    
    L::flush();

    if (fileContains(logFile, "Unique Message 1 (x")) {
        std::cerr << "Unique Message 1 should not be collapsed!" << std::endl;
        return 1;
    }

     // Test interleaved
    L().info() << "A";
    L().info() << "A";
    L().info() << "B";
    L().info() << "A";
    L().info() << "A";
    
    L::flush();
    
    // Should see A (x4), B
    if (!fileContains(logFile, "A (x4)")) {
         std::cerr << "File should contain 'A (x4)'!" << std::endl;
         return 1;
    }
    if (fileContains(logFile, "A (x2)")) {
         std::cerr << "File should NOT contain 'A (x2)'!" << std::endl;
         return 1;
    }

    std::cout << "Test passed!" << std::endl;
    return 0;
}
