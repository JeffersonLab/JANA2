//
// Created by nbrei on 3/28/19.
//

#ifndef JANA2_PERFUTILS_H
#define JANA2_PERFUTILS_H

#include <vector>
#include <random>

extern thread_local std::mt19937* generator;



uint64_t consume_cpu_ms(long millisecs);

int randint(int min, int max);

double randdouble();

size_t writeMemory(std::vector<char>& buffer, size_t count);

double writeMemory(std::vector<double>& buffer, size_t count);

size_t readMemory(std::vector<char>& buffer, size_t count);

double readMemory(std::vector<double>& buffer, size_t count);

void touchMemory(std::vector<char>* buffer, size_t count);

double doBusyWork(std::vector<double>& buffer, double input);


#endif // JANA2_PERFUTILS_H
