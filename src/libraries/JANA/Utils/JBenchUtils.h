
// Copyright 2020, Jefferson Science Associates, LLC.
// Subject to the terms in the LICENSE file found in the top-level directory.

#pragma once

#include <vector>
#include <random>
#include <string>
#include <algorithm>
#include <string>
#include <typeinfo>
#include <chrono>


class JBenchUtils {

    std::mt19937 m_generator;

public:

    JBenchUtils(){}

    void set_seed(size_t event_number, std::string caller_name);

    size_t rand_size(size_t avg, double spread);
    int randint(int min, int max);
    double randdouble(double min=0.0, double max=1000.0);
    float randfloat(float min=0.0, float max=1000.0);

    uint64_t consume_cpu_ms(uint64_t millisecs, double spread=0.0, bool fix_flops=true);
    uint64_t read_memory(const std::vector<char>& buffer);
    uint64_t write_memory(std::vector<char>& buffer, uint64_t bytes, double spread=0.0);

};
