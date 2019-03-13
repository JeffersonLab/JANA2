#pragma once

#include <vector>
#include <random>
#include <thread>

void consumeCPU(long microsecs) {}

int randint(int min, int max) {

    static thread_local std::mt19937* generator = nullptr;
    if (!generator) generator = new std::mt19937(clock() + std::this_thread::get_id().hash());
    std::uniform_int_distribution<int> distribution(min, max);
    return distribution(*generator);
}


size_t fillMemory(std::vector<double>* buffer, size_t count) {

    buffer->reserve(count);
    size_t sum;
    for (int i=0; i<count; ++i) {
        int x = randint(0, 100);
        (*buffer)[i] = x;
        sum += x;
    }
    return sum;
}

size_t readMemory(std::vector<double>* buffer, size_t count) {

    size_t sum = 0;
    for (int i=0; i<count; ++i) {
        sum += (*buffer)[i];
    }
    return sum;
};

void touchMemory(std::vector<double>* buffer, size_t count) {


};

