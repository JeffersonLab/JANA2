//
// Created by nbrei on 3/28/19.
//

#include <thread>
#include <algorithm>

#include "JPerfUtils.h"


thread_local std::mt19937* generator = nullptr;

uint64_t consume_cpu_ms(uint64_t millisecs, double spread) {

    uint64_t sampled = rand_size(millisecs, spread);
    auto duration = std::chrono::milliseconds(sampled);

    auto start_time = std::chrono::steady_clock::now();
    uint64_t result = 0;

    while ((std::chrono::steady_clock::now() - start_time) < duration) {

        for (int x=0; x<randint(10,1000); ++x) {
            result++;
        }
    }
    return result;
}

uint64_t read_memory(const std::vector<char>& buffer) {

    auto length = buffer.size();
    uint64_t sum = 0;
    for (int i=0; i<length; ++i) {
        sum += buffer[i];
    }
    return sum;
}

uint64_t write_memory(std::vector<char>& buffer, uint64_t bytes, double spread) {

    uint64_t sampled = rand_size(bytes, spread);
    for (int i=0; i<sampled; ++i) {
        buffer.push_back(2);
    }
    return sampled*2;
}

size_t rand_size(size_t avg, double spread) {

    auto delta = static_cast<size_t>(avg*spread);

    if (!generator) {
        std::hash<std::thread::id> hasher;
        long seed = clock() + hasher(std::this_thread::get_id());
        generator = new std::mt19937(seed);
    }
    std::uniform_int_distribution<int> distribution(avg-delta, avg+delta);
    return distribution(*generator);
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


size_t writeMemory(std::vector<char> &buffer, size_t bytes) {

    size_t sum = 0;
    for (size_t i = 0; i < bytes; ++i) {
        char x = randchar(0, 255);
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

double writeMemory(std::vector<double>& buffer, size_t count) {

    double sum = 0;
    for (unsigned i=0; i<count; ++i) {
        int x = randint(0, 100);
        buffer.push_back(x);
        sum += x;
    }
    return sum;
}

double readMemory(std::vector<double>& buffer, size_t count) {

    count = std::min(count, buffer.size());
    double sum = 0;
    for (unsigned i=0; i < count; ++i) {
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


