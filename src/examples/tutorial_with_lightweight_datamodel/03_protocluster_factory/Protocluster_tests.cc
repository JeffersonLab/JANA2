
#include "CalorimeterCluster.h"
#include "CalorimeterHit.h"
#include "JANA/JApplicationFwd.h"
#include "JANA/JFactoryGenerator.h"
#include "JANA/JEvent.h"
#define CATCH_CONFIG_MAIN
#include <catch.hpp>
#include "Protocluster_algorithm.h"
#include "Protocluster_factory.h"
#include "Protocluster_factory_gluex.h"
#include "Protocluster_factory_epic.h"


std::vector<const CalorimeterHit*> populateHits(size_t rows, size_t cols, std::vector<double> energies) {

    REQUIRE(energies.size() == rows*cols);
    std::vector<const CalorimeterHit*> results;
    size_t energies_index = 0;
    for (size_t row=0; row<rows; ++row) {
        for (size_t col=0;  col<cols; ++col) {
            if (energies[energies_index] > 0.0) {
                CalorimeterHit* hit = new CalorimeterHit(0,0,0,0,0,0,0,0);
                hit->energy = energies[energies_index];
                hit->row = row;
                hit->col = col;
                hit->x = row * 1.0;
                hit->y = col * 1.0;
                results.push_back(hit);
            }
            energies_index += 1;
        }
    }
    return results;
};

void printCluster(CalorimeterCluster& cluster) {
    std::cout << "CalorimeterCluster " << &cluster << " with energy=" << cluster.energy
              << ", x=" << cluster.x_center << ", y=" << cluster.y_center << std::endl;
    auto hits = cluster.Get<CalorimeterHit>();
    for (auto hit : hits) {
        std::cout << "  " << "energy=" << hit->energy << ", row=" << hit->row << ", col=" << hit->col << std::endl;
    }
}

// Your algorithm can be broken up into smaller pieces.
// This can make it easier to test.
TEST_CASE("UnionFind_tests") {
    UnionFind f(3);
    f.unite(1, 2);
    int x = f.find(1);
    int y = f.find(2);
    REQUIRE(x == y);

    int z = f.find(0);
    REQUIRE(z != x);
    REQUIRE(z != y);

    f.unite(2, 0);
    int r0 = f.find(0);
    int r1 = f.find(1);
    int r2 = f.find(2);
    REQUIRE(r0 == r1);
    REQUIRE(r1 == r2);
}

// Prefer testing the algorithm directly, when that makes sense.
TEST_CASE("Protocluster_algorithm_tests") {
    std::vector<CalorimeterCluster*> clusters;

    // ----------------------
    // No hits => No clusters
    // ----------------------
    std::vector<const CalorimeterHit*> empty_hits;
    auto empty_clusters = calculate_protoclusters(empty_hits, 4.0);
    REQUIRE(empty_clusters.size() == 0);

    // ----------------------
    // One hit => one clusters
    // ----------------------
    std::vector<const CalorimeterHit*> single_hit;
    CalorimeterHit* hit = new CalorimeterHit(0,0,0,0,0,0,0,0);
    hit->energy = 100;
    hit->row = 4;
    hit->col = 4;
    hit->x = 1.0;
    hit->y = 1.0;

    single_hit.push_back(hit);
    auto single_cluster = calculate_protoclusters(single_hit, 4.0);
    REQUIRE(single_cluster.size() == 1);
    REQUIRE(single_cluster.at(0)->energy == 100);
    REQUIRE(single_cluster.at(0)->x_center == 1.0);
    REQUIRE(single_cluster.at(0)->y_center == 1.0);


    // ----------------------
    // Symmetric blob of hits => Cluster with exact center
    // ----------------------
    std::vector<const CalorimeterHit*> symmetric_hits = populateHits(8, 8, { 
        0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0,
        0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0,
        0.0, 0.0, 4.0, 4.0, 4.0, 0.0, 0.0, 0.0,
        0.0, 0.0, 4.0, 8.0, 4.0, 0.0, 0.0, 0.0,
        0.0, 0.0, 4.0, 4.0, 4.0, 0.0, 0.0, 0.0,
        0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0,
        0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0,
        0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0,
    });
    auto symmetric_cluster = calculate_protoclusters(symmetric_hits, 5.0);
    /*
    for (auto cluster : symmetric_cluster) {
        printCluster(*cluster);
    }
    */
    REQUIRE(symmetric_cluster.size() == 1);
    REQUIRE(symmetric_cluster.at(0)->energy == 40.0);
    REQUIRE_THAT(symmetric_cluster.at(0)->x_center, Catch::Matchers::WithinRel(3.0));
    REQUIRE_THAT(symmetric_cluster.at(0)->y_center, Catch::Matchers::WithinRel(3.0));


    // ----------------------
    // Two distinct clusters
    // ----------------------
    std::vector<const CalorimeterHit*> two_cluster_hits = populateHits(8, 8, { 
        0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0,
        0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0,
        1.0, 1.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0,
        1.0, 1.0, 0.0, 4.0, 4.0, 0.0, 0.0, 0.0,
        0.0, 0.0, 0.0, 4.0, 4.0, 0.0, 0.0, 0.0,
        0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0,
        0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0,
        0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0,
    });
    auto two_clusters = calculate_protoclusters(two_cluster_hits, 5.0);
    /*
    for (auto cluster : two_clusters) {
        printCluster(*cluster);
    }
    */
    REQUIRE(two_clusters.size() == 2);
    REQUIRE_THAT(two_clusters.at(0)->energy, Catch::Matchers::WithinRel(4.0));
    REQUIRE_THAT(two_clusters.at(1)->energy, Catch::Matchers::WithinRel(16.0));
    REQUIRE_THAT(two_clusters.at(0)->x_center, Catch::Matchers::WithinRel(2.5));
    REQUIRE_THAT(two_clusters.at(0)->y_center, Catch::Matchers::WithinRel(0.5));
    REQUIRE_THAT(two_clusters.at(1)->x_center, Catch::Matchers::WithinRel(3.5));
    REQUIRE_THAT(two_clusters.at(1)->y_center, Catch::Matchers::WithinRel(3.5));
};


std::vector<CalorimeterHit*> make_two_cluster_hits() {
    std::vector<CalorimeterHit*> two_cluster_hits;
    two_cluster_hits.push_back(new CalorimeterHit(0, 2, 0, 0.0, 2.0, 0.0, 2.0, 0.0));
    two_cluster_hits.push_back(new CalorimeterHit(0, 3, 0, 0.0, 3.0, 0.0, 3.0, 0.0));
    two_cluster_hits.push_back(new CalorimeterHit(0, 2, 1, 1.0, 2.0, 0.0, 4.0, 0.0));
    two_cluster_hits.push_back(new CalorimeterHit(0, 3, 1, 1.0, 3.0, 0.0, 5.0, 0.0));

    two_cluster_hits.push_back(new CalorimeterHit(0, 6, 3, 3.0, 6.0, 0.0, 6.0, 0.0));
    two_cluster_hits.push_back(new CalorimeterHit(0, 6, 4, 4.0, 6.0, 0.0, 7.0, 0.0));
    return two_cluster_hits;
}

// If you need to incorporate other JFactories into your test case, you can do it like so:
TEST_CASE("Protocluster_factory_tests") {
    // ----------------------
    // Two distinct clusters
    // ----------------------

    JApplication app;
    app.Add(new JFactoryGeneratorT<Protocluster_factory>());
    auto event = std::make_shared<JEvent>(&app);

    event->Insert(make_two_cluster_hits(), "");
    auto clusters = event->Get<CalorimeterCluster>("proto");

    REQUIRE(clusters.size() == 2);
    REQUIRE_THAT(clusters.at(0)->energy, Catch::Matchers::WithinRel(14.0));
    REQUIRE_THAT(clusters.at(1)->energy, Catch::Matchers::WithinRel(13.0));
}

TEST_CASE("Protocluster_factory_epic_tests") {
    // ----------------------
    // Two distinct clusters
    // ----------------------

    JApplication app;
    app.Add(new JFactoryGeneratorT<Protocluster_factory_epic>());
    auto event = std::make_shared<JEvent>(&app);

    event->Insert(make_two_cluster_hits(), "");
    auto clusters = event->Get<CalorimeterCluster>("proto");

    REQUIRE(clusters.size() == 2);
    REQUIRE_THAT(clusters.at(0)->energy, Catch::Matchers::WithinRel(14.0));
    REQUIRE_THAT(clusters.at(1)->energy, Catch::Matchers::WithinRel(13.0));
}

TEST_CASE("Protocluster_factory_gluex_tests") {
    // ----------------------
    // Two distinct clusters
    // ----------------------

    JApplication app;
    app.Add(new JFactoryGeneratorT<Protocluster_factory_gluex>());
    auto event = std::make_shared<JEvent>(&app);

    event->Insert(make_two_cluster_hits(), "");
    auto clusters = event->Get<CalorimeterCluster>("proto");

    REQUIRE(clusters.size() == 2);
    REQUIRE_THAT(clusters.at(0)->energy, Catch::Matchers::WithinRel(14.0));
    REQUIRE_THAT(clusters.at(1)->energy, Catch::Matchers::WithinRel(13.0));
}

