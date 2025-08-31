

#include "jana2_tutorial_podio_datamodel/CalorimeterHit.h"
#include "jana2_tutorial_podio_datamodel/CalorimeterHitCollection.h"
#include "jana2_tutorial_podio_datamodel/CalorimeterClusterCollection.h"

#include "JANA/JFactoryGenerator.h"
#include "JANA/JEvent.h"
#define CATCH_CONFIG_MAIN
#include <catch.hpp>
#include "Protocluster_algorithm.h"
#include "Protocluster_factory.h"
#include "Protocluster_factory_epic.h"


CalorimeterHitCollection populateHits(size_t rows, size_t cols, std::vector<double> energies) {

    REQUIRE(energies.size() == rows*cols);
    CalorimeterHitCollection results;

    size_t energies_index = 0;
    for (size_t row=0; row<rows; ++row) {
        for (size_t col=0;  col<cols; ++col) {
            if (energies[energies_index] > 0.0) {
                MutableCalorimeterHit hit;
                hit.setEnergy(energies[energies_index]);
                hit.setRow(row);
                hit.setCol(col);
                hit.setX(row * 1.0);
                hit.setY(col * 1.0);
                results.push_back(hit);
            }
            energies_index += 1;
        }
    }
    return results;
};

void printCluster(CalorimeterCluster& cluster) {
    std::cout << "CalorimeterCluster " << " with energy=" << cluster.getEnergy()
              << ", x=" << cluster.getX_center() << ", y=" << cluster.getY_center() << std::endl;
    for (const auto& hit : cluster.getHits()) {
        std::cout << "  " << "energy=" << hit.getEnergy() << ", row=" << hit.getRow() << ", col=" << hit.getCol() << std::endl;
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
    CalorimeterHitCollection empty_hits;
    auto empty_clusters = calculate_protoclusters(&empty_hits, 4.0);
    REQUIRE(empty_clusters.size() == 0);

    // ----------------------
    // One hit => one clusters
    // ----------------------
    CalorimeterHitCollection single_hit;
    MutableCalorimeterHit hit;
    hit.setEnergy(100);
    hit.setRow(4);
    hit.setCol(4);
    hit.setX(1.0);
    hit.setY(1.0);

    single_hit.push_back(hit);

    auto single_cluster = calculate_protoclusters(&single_hit, 4.0);
    REQUIRE(single_cluster.size() == 1);
    REQUIRE(single_cluster.at(0).getEnergy() == 100);
    REQUIRE(single_cluster.at(0).getX_center() == 1.0);
    REQUIRE(single_cluster.at(0).getY_center() == 1.0);


    // ----------------------
    // Symmetric blob of hits => Cluster with exact center
    // ----------------------
    auto symmetric_hits = populateHits(8, 8, { 
        0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0,
        0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0,
        0.0, 0.0, 4.0, 4.0, 4.0, 0.0, 0.0, 0.0,
        0.0, 0.0, 4.0, 8.0, 4.0, 0.0, 0.0, 0.0,
        0.0, 0.0, 4.0, 4.0, 4.0, 0.0, 0.0, 0.0,
        0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0,
        0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0,
        0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0,
    });
    auto symmetric_cluster = calculate_protoclusters(&symmetric_hits, 5.0);
    /*
    for (auto cluster : symmetric_cluster) {
        printCluster(*cluster);
    }
    */
    REQUIRE(symmetric_cluster.size() == 1);
    REQUIRE_THAT(symmetric_cluster.at(0).getEnergy(), Catch::Matchers::WithinRel(40.0));
    REQUIRE_THAT(symmetric_cluster.at(0).getX_center(), Catch::Matchers::WithinRel(3.0));
    REQUIRE_THAT(symmetric_cluster.at(0).getY_center(), Catch::Matchers::WithinRel(3.0));


    // ----------------------
    // Two distinct clusters
    // ----------------------
    auto two_cluster_hits = populateHits(8, 8, { 
        0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0,
        0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0,
        1.0, 1.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0,
        1.0, 1.0, 0.0, 4.0, 4.0, 0.0, 0.0, 0.0,
        0.0, 0.0, 0.0, 4.0, 4.0, 0.0, 0.0, 0.0,
        0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0,
        0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0,
        0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0,
    });
    auto two_clusters = calculate_protoclusters(&two_cluster_hits, 5.0);
    /*
    for (auto cluster : two_clusters) {
        printCluster(*cluster);
    }
    */
    REQUIRE(two_clusters.size() == 2);
    REQUIRE_THAT(two_clusters.at(0).getEnergy(), Catch::Matchers::WithinRel(4.0));
    REQUIRE_THAT(two_clusters.at(1).getEnergy(), Catch::Matchers::WithinRel(16.0));
    REQUIRE_THAT(two_clusters.at(0).getX_center(), Catch::Matchers::WithinRel(2.5));
    REQUIRE_THAT(two_clusters.at(0).getY_center(), Catch::Matchers::WithinRel(0.5));
    REQUIRE_THAT(two_clusters.at(1).getX_center(), Catch::Matchers::WithinRel(3.5));
    REQUIRE_THAT(two_clusters.at(1).getY_center(), Catch::Matchers::WithinRel(3.5));
};


// If you need to incorporate other JFactories into your test case, you can do it like so:
TEST_CASE("Protocluster_factory_tests") {
    // ----------------------
    // Two distinct clusters
    // ----------------------

    JApplication app;
    app.Add(new JFactoryGeneratorT<Protocluster_factory>());
    auto event = std::make_shared<JEvent>(&app);

    CalorimeterHitCollection two_cluster_hits;
    two_cluster_hits.push_back(MutableCalorimeterHit(0, 2, 0, 0.0, 2.0, 0.0, 2.0, 0.0));
    two_cluster_hits.push_back(MutableCalorimeterHit(0, 3, 0, 0.0, 3.0, 0.0, 3.0, 0.0));
    two_cluster_hits.push_back(MutableCalorimeterHit(0, 2, 1, 1.0, 2.0, 0.0, 4.0, 0.0));
    two_cluster_hits.push_back(MutableCalorimeterHit(0, 3, 1, 1.0, 3.0, 0.0, 5.0, 0.0));

    two_cluster_hits.push_back(MutableCalorimeterHit(0, 6, 3, 3.0, 6.0, 0.0, 6.0, 0.0));
    two_cluster_hits.push_back(MutableCalorimeterHit(0, 6, 4, 4.0, 6.0, 0.0, 7.0, 0.0));

    event->InsertCollection<CalorimeterHit>(std::move(two_cluster_hits), "CalorimeterHit");
    auto clusters = event->GetCollection<CalorimeterCluster>("CalorimeterCluster:proto");

    REQUIRE(clusters->size() == 2);
    REQUIRE_THAT(clusters->at(0).getEnergy(), Catch::Matchers::WithinRel(14.0));
    REQUIRE_THAT(clusters->at(1).getEnergy(), Catch::Matchers::WithinRel(13.0));
}

TEST_CASE("Protocluster_factory_epic_tests") {
    // ----------------------
    // Two distinct clusters
    // ----------------------

    JApplication app;
    app.Add(new JFactoryGeneratorT<Protocluster_factory_epic>());
    auto event = std::make_shared<JEvent>(&app);

    CalorimeterHitCollection two_cluster_hits;
    two_cluster_hits.push_back(MutableCalorimeterHit(0, 2, 0, 0.0, 2.0, 0.0, 2.0, 0.0));
    two_cluster_hits.push_back(MutableCalorimeterHit(0, 3, 0, 0.0, 3.0, 0.0, 3.0, 0.0));
    two_cluster_hits.push_back(MutableCalorimeterHit(0, 2, 1, 1.0, 2.0, 0.0, 4.0, 0.0));
    two_cluster_hits.push_back(MutableCalorimeterHit(0, 3, 1, 1.0, 3.0, 0.0, 5.0, 0.0));

    two_cluster_hits.push_back(MutableCalorimeterHit(0, 6, 3, 3.0, 6.0, 0.0, 6.0, 0.0));
    two_cluster_hits.push_back(MutableCalorimeterHit(0, 6, 4, 4.0, 6.0, 0.0, 7.0, 0.0));

    event->InsertCollection<CalorimeterHit>(std::move(two_cluster_hits), "CalorimeterHit");
    auto clusters = event->GetCollection<CalorimeterCluster>("CalorimeterCluster:proto");

    REQUIRE(clusters->size() == 2);
    REQUIRE_THAT(clusters->at(0).getEnergy(), Catch::Matchers::WithinRel(14.0));
    REQUIRE_THAT(clusters->at(1).getEnergy(), Catch::Matchers::WithinRel(13.0));
}

