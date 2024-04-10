
#include "CollectionTabulators.h"

JTablePrinter TabulateClusters(const ExampleClusterCollection* c) {
    
    JTablePrinter t;
    t.AddColumn("clusterId");
    t.AddColumn("energy");
    t.AddColumn("hits");
    t.AddColumn("clusters");

    for (auto cluster : *c) {
        std::ostringstream oss;
        for (auto hit : cluster.Hits()) {
            oss << hit.id() << " ";
        }
        std::ostringstream oss2;
        for (auto cluster : cluster.Clusters()) {
            oss2 << cluster.id() << " ";
        }
        t | cluster.id() | cluster.energy() | oss.str() | oss2.str();
    }
    return t;
}


JTablePrinter TabulateHits(const ExampleHitCollection* c) {

    JTablePrinter t;
    t.AddColumn("hitId");
    t.AddColumn("cellId");
    t.AddColumn("x");
    t.AddColumn("y");
    t.AddColumn("energy");
    t.AddColumn("time");

    for (auto hit : *c) {
        t | hit.id() | hit.cellID() | hit.x() | hit.y() | hit.energy() | hit.time();
    }
    return t;
}
