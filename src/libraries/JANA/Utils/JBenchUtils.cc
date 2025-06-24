
// Copyright 2020, Jefferson Science Associates, LLC.
// Subject to the terms in the LICENSE file found in the top-level directory.


#include "JBenchUtils.h"
#include <chrono>


void JBenchUtils::set_seed(size_t event_number, std::string caller_name)
{
    std::hash<std::string> hasher;
    long seed = event_number ^ hasher(caller_name);
    m_generator = std::mt19937(seed);
}


size_t JBenchUtils::rand_size(size_t avg, double spread) {
    auto delta = static_cast<size_t>(avg*spread);
    std::uniform_int_distribution<size_t> distribution(avg-delta, avg+delta);
    return distribution(m_generator);
}


int JBenchUtils::randint(int min, int max) {
    std::uniform_int_distribution<int> distribution(min, max);
    return distribution(m_generator);
}

double JBenchUtils::randdouble(double min, double max) {
    std::uniform_real_distribution<double> dist(min, max);
    return dist(m_generator);
}

float JBenchUtils::randfloat(float min, float max) {
    std::uniform_real_distribution<float> dist(min, max);
    return dist(m_generator);
}

uint64_t JBenchUtils::consume_cpu_ms(uint64_t millisecs, double spread) {

    uint64_t sampled = rand_size(millisecs, spread);
    uint64_t result = 0;

    // Perform a variable amount of work in a fixed time
    auto duration = std::chrono::milliseconds(sampled);
    auto start_time = std::chrono::steady_clock::now();
    while ((std::chrono::steady_clock::now() - start_time) < duration) {

        double a = (m_generator)();
        double b = sqrt(a * pow(1.23, -a)) / a;
        result += long(b);
    }
    return result;
}

uint64_t JBenchUtils::read_memory(const std::vector<char>& buffer) {

    auto length = buffer.size();
    uint64_t sum = 0;
    for (unsigned i=0; i<length; ++i) {
        sum += buffer[i];
    }
    return sum;
}

uint64_t JBenchUtils::write_memory(std::vector<char>& buffer, uint64_t bytes, double spread) {

    uint64_t sampled = rand_size(bytes, spread);
    for (unsigned i=0; i<sampled; ++i) {
        buffer.push_back(2);
    }
    return sampled*2;
}





