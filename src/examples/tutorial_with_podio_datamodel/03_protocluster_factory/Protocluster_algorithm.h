
#pragma once
#include <jana2_tutorial_podio_datamodel/CalorimeterHitCollection.h>
#include <jana2_tutorial_podio_datamodel/CalorimeterClusterCollection.h>


class UnionFind {
    std::vector<int> parent;
    std::vector<int> rank;
public:
    UnionFind(size_t item_count);
    int find(int x);
    void unite(int a, int b);
};


CalorimeterClusterCollection calculate_protoclusters(const CalorimeterHitCollection* hits,
                                                     double log_weight_reference_energy);

