
#pragma once
#include <CalorimeterHit.h>
#include <CalorimeterCluster.h>


class UnionFind {
    std::vector<int> parent;
    std::vector<int> rank;
public:
    UnionFind(size_t item_count);
    int find(int x);
    void unite(int a, int b);
};


std::vector<CalorimeterCluster*> calculate_protoclusters(
    const std::vector<const CalorimeterHit*> hits,
    double log_weight_reference_energy);

