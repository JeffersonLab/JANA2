#pragma once

#include <vector>
#include <random>
#include <thread>
#include <algorithm>

void consumeCPU(long microsecs) {}

static thread_local std::mt19937* generator = nullptr;

int randint(int min, int max) {

    std::hash<std::thread::id> hasher;
    long seed = clock() + hasher(std::this_thread::get_id());
    if (!generator) generator = new std::mt19937(seed);
    std::uniform_int_distribution<int> distribution(min, max);
    return distribution(*generator);
}

double randdouble() {
    return (*generator)();
}

size_t writeMemory(std::vector<double>& buffer, size_t count) {

    size_t sum;
    for (int i=0; i<count; ++i) {
        int x = randint(0, 100);
        buffer.push_back(x);
        sum += x;
    }
    return sum;
}

size_t readMemory(std::vector<double>& buffer, size_t count) {

    count = std::min(count, buffer.size());
    size_t sum = 0;
    for (int i=0; i < count; ++i) {
        sum += buffer[i];
    }
    return sum;
};

void touchMemory(std::vector<double>* buffer, size_t count) {


};

double doBusyWork(std::vector<double>& buffer, double input) {
    /// TODO: Replace this with more calibrated consumeCPU() and touchMemory()

    double c = input;
    for (int i = 0; i < 1000; i++) {
        double a = randdouble();
        double b = sqrt(a * pow(1.23, -a)) / a;
        c += b;
        buffer.push_back(a); // add to jana_test object
    }
    return c;
}




