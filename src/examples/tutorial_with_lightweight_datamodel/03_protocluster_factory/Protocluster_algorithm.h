
#pragma once
#include <CalorimeterHit.h>
#include <CalorimeterCluster.h>

class Protocluster_algorithm {
private:

public:
    std::vector<CalorimeterCluster*> Execute(const std::vector<const CalorimeterHit*> hits, 
                                             double energy_threshold) const;
};
