
// Copyright 2020, Jefferson Science Associates, LLC.
// Subject to the terms in the LICENSE file found in the top-level directory.


#include <thread>
#include <algorithm>

#include "JPerfUtils.h"


thread_local std::mt19937* generator = nullptr;

uint64_t consume_cpu_ms(uint64_t millisecs, double spread, bool fix_flops) {

    uint64_t sampled = rand_size(millisecs, spread);
    uint64_t result = 0;

    if (fix_flops) {
        // Perform a fixed amount of work in a variable time
        const uint64_t appx_iters_per_millisec = 14000;
        sampled *= appx_iters_per_millisec;

        for (uint64_t i=0; i<sampled; ++i) {
            double a = (*generator)();
            double b = sqrt(a * pow(1.23, -a)) / a;
            result += long(b);
        }
    }
    else {
        // Perform a variable amount of work in a fixed time
        auto duration = std::chrono::milliseconds(sampled);
        auto start_time = std::chrono::steady_clock::now();
        while ((std::chrono::steady_clock::now() - start_time) < duration) {

            double a = (*generator)();
            double b = sqrt(a * pow(1.23, -a)) / a;
            result += long(b);
        }
    }
    return result;
}

uint64_t read_memory(const std::vector<char>& buffer) {

    auto length = buffer.size();
    uint64_t sum = 0;
    for (unsigned i=0; i<length; ++i) {
        sum += buffer[i];
    }
    return sum;
}

uint64_t write_memory(std::vector<char>& buffer, uint64_t bytes, double spread) {

    uint64_t sampled = rand_size(bytes, spread);
    for (unsigned i=0; i<sampled; ++i) {
        buffer.push_back(2);
    }
    return sampled*2;
}

void init_generator() {
    if (!generator) {
        std::hash<std::thread::id> hasher;
        long now = std::chrono::steady_clock::now().time_since_epoch().count();
        long seed = now + hasher(std::this_thread::get_id());
        generator = new std::mt19937(seed);
    }
}

size_t rand_size(size_t avg, double spread) {
    auto delta = static_cast<size_t>(avg*spread);
    init_generator();
    std::uniform_int_distribution<size_t> distribution(avg-delta, avg+delta);
    return distribution(*generator);
}


int randint(int min, int max) {
    init_generator();
    std::uniform_int_distribution<int> distribution(min, max);
    return distribution(*generator);
}

double randdouble(double min, double max) {
    init_generator();
    std::uniform_real_distribution<double> dist(min, max);
    return dist(*generator);
}

float randfloat(float min, float max) {
    init_generator();
    std::uniform_real_distribution<float> dist(min, max);
    return dist(*generator);
}



