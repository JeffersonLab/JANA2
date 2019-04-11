//
// Created by nbrei on 3/28/19.
//

#include <thread>
#include <algorithm>

#include "JPerfUtils.h"


thread_local std::mt19937* generator = nullptr;

uint64_t consume_cpu_ms(long millisecs) {

    auto start_time = std::chrono::steady_clock::now();
    auto duration = std::chrono::milliseconds(millisecs);
    uint64_t result = 0;

    while ((std::chrono::steady_clock::now() - start_time) < duration) {

        for (int x=0; x<randint(10,1000); ++x) {
            result++;
        }
    }
    return result;
}


int randint(int min, int max) {

    std::hash<std::thread::id> hasher;
    long seed = clock() + hasher(std::this_thread::get_id());
    if (!generator) generator = new std::mt19937(seed);
    std::uniform_int_distribution<int> distribution(min, max);
    return distribution(*generator);
}

int randchar(char min, char max) {

    std::hash<std::thread::id> hasher;
    long seed = clock() + hasher(std::this_thread::get_id());
    if (!generator) generator = new std::mt19937(seed);
    std::uniform_int_distribution<char> distribution(min, max);
    return distribution(*generator);
}

double randdouble() {
    return (*generator)();
}


size_t writeMemory(std::vector<char> &buffer, size_t count) {

    size_t sum = 0;
    for (size_t i = 0; i < count; ++i) {
        char x = randchar(-128, 127);
        buffer.push_back(x);
        sum += x;
    }
    return sum;
}


size_t readMemory(std::vector<char> &buffer, size_t count) {

    count = std::min(count, buffer.size());
    size_t sum = 0;
    for (size_t i = 0; i < count; ++i) {
        sum += buffer[i];
    }
    return sum;
};


void touchMemory(std::vector<double> *buffer, size_t count) {

    for (size_t i = 0; i < count; ++i) {
        (*buffer)[i]++;
    }
};


double doBusyWork(std::vector<double> &buffer, double input) {

    double c = input;
    for (size_t i = 0; i < 1000; i++) {
        double a = randdouble();
        double b = sqrt(a * pow(1.23, -a)) / a;
        c += b;
        buffer.push_back(a); // add to jana_test object
    }
    return c;
}


