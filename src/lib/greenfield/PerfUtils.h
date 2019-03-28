//
// Created by nbrei on 3/28/19.
//

#ifndef JANA2_PERFUTILS_H
#define JANA2_PERFUTILS_H

#include <vector>
#include <random>

extern thread_local std::mt19937* generator;


namespace greenfield {


uint64_t consumeCPU(long microsecs);

int randint(int min, int max);

double randdouble();

size_t writeMemory(std::vector<double>& buffer, size_t count);

size_t readMemory(std::vector<double>& buffer, size_t count);

void touchMemory(std::vector<double>* buffer, size_t count);

double doBusyWork(std::vector<double>& buffer, double input);


} // namespace greenfield

#endif // JANA2_PERFUTILS_H
