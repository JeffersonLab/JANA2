
#include "Protocluster_algorithm.h"
#include "CalorimeterCluster.h"
#include <map>
#include <cmath>
#include <algorithm>

UnionFind::UnionFind(size_t item_count) {
    parent.reserve(item_count);
    rank.reserve(item_count);
    for (size_t i=0; i<item_count; ++i) {
        parent[i] = i;
        rank[i] = 0;
    }
}

int UnionFind::find(int x) {
    // Follow parent pointers to find "root" parent
    if (parent[x] != x) {
        parent[x] = find(parent[x]);
    }
    return parent[x];
}

void UnionFind::unite(int a, int b) {
    int ra = find(a);
    int rb = find(b);
    if (ra == rb) return;

    if (rank[ra] < rank[rb]) {
        parent[ra] = rb;
    }
    else if (rank[ra] > rank[rb]) {
        parent[rb] = ra;
    }
    else {
        parent[rb] = ra;
        rank[ra]++;
    }
}

std::vector<CalorimeterCluster*> calculate_protoclusters(const std::vector<const CalorimeterHit*> hits, 
                                                         double log_weight_reference_energy) {

    std::map<std::pair<int, int>, int> cell_to_hit_index;

    for (size_t hit_index=0; hit_index < hits.size(); ++hit_index) {
        // Build map from cell row,col pairs to hit indices so that we can look up neighbors
        auto hit = hits[hit_index];
        cell_to_hit_index[{hit->row, hit->col}] = hit_index++;
    }

    UnionFind union_find_alg(hits.size());

    for (auto& pair : cell_to_hit_index) {
        // Neighbor indices are already sorted, so we don't have to look in all 8 directions
        auto row = pair.first.first;
        auto col = pair.first.second;
        auto idx = pair.second;

        auto north = cell_to_hit_index.find({row-1, col});
        if (north != cell_to_hit_index.end()) {
            union_find_alg.unite(idx, north->second);
        }
        auto east = cell_to_hit_index.find({row, col-1});
        if (east != cell_to_hit_index.end()) {
            union_find_alg.unite(idx, east->second);
        }
        auto northeast = cell_to_hit_index.find({row-1, col-1});
        if (northeast != cell_to_hit_index.end()) {
            union_find_alg.unite(idx, northeast->second);
        }
    }

    // All of the roots have been found at this point

    std::map<int, CalorimeterCluster*> root_to_clusters;

    for (size_t hit_index=0; hit_index < hits.size(); ++hit_index) {

        auto hit = hits[hit_index];
        auto root = union_find_alg.find(hit_index);

        // Retrieve or create cluster corresponding to this root
        CalorimeterCluster* cluster = nullptr;
        auto it = root_to_clusters.find(root);
        if (it == root_to_clusters.end()) {
            cluster = new CalorimeterCluster;
            root_to_clusters[root] = cluster;
        }
        else {
            cluster = it->second;
        }

        // Update cluster with hit information
        cluster->energy += hit->energy;
        cluster->time_begin = std::min(hit->time, cluster->time_begin);
        cluster->time_end = std::max(hit->time, cluster->time_end);
        cluster->AddAssociatedObject(hit);
    }

    std::vector<CalorimeterCluster*> clusters_out;
    for (auto& it : root_to_clusters) {
        // Calculated log-weighted centroid
        // w_i = max(0, w_0 + ln(E_i/E_cl))
        // r_cl = sum_i(w_i r_i) / sum_i(w_i))

        double w_0 = log_weight_reference_energy; // 4.2 for CMS
        double r_x = 0;
        double r_y = 0;
        double sum_w_i = 0;

        auto* cluster = it.second;
        auto hits = cluster->Get<CalorimeterHit>();
        for (auto* hit : hits) {
            if (hit->energy < 0) continue;
            double w_i = w_0 + std::log(hit->energy/cluster->energy);
            if (w_i <= 0) continue;
            sum_w_i += w_i;
            r_x += w_i * hit->x;
            r_y += w_i * hit->y;
        }
        r_x /= sum_w_i;
        r_y /= sum_w_i;
        cluster->x_center = r_x;
        cluster->y_center = r_y;

        clusters_out.push_back(it.second);
    }

    return clusters_out;
};


