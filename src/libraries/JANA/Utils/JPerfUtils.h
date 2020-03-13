//
// Created by nbrei on 3/28/19.
//

#ifndef JANA2_PERFUTILS_H
#define JANA2_PERFUTILS_H

#include <vector>
#include <random>

extern thread_local std::mt19937* generator;


uint64_t consume_cpu_ms(uint64_t millisecs, double spread=0.0, bool fix_flops=true);

uint64_t read_memory(const std::vector<char>& buffer);

uint64_t write_memory(std::vector<char>& buffer, uint64_t bytes, double spread=0.0);

size_t rand_size(size_t avg, double spread);

int randint(int min, int max);

double randdouble(double min=0.0, double max=1000.0);

float randfloat(float min=0.0, float max=1000.0);
#endif // JANA2_PERFUTILS_H
