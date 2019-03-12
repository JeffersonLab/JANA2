#pragma once


void consumeCPU(long seconds) {}

void fillMemory(std::vector<double>* buffer, size_t count, size_t sumsTo);

void readMemory(std::vector<double>* buffer, size_t count);

void touchMemory(std::vector<double>* buffer, size_t count) {

};

size_t randint(size_t lower, size_t upper);
