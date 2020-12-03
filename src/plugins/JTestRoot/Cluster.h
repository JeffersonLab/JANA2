// Copyright 2020, Jefferson Science Associates, LLC.
// Subject to the terms in the LICENSE file found in the top-level directory.

#ifndef _CLUSTER_H_
#define _CLUSTER_H_

#include <vector>
#include <TObject.h>
#include "Hit.h"

class Cluster:public TObject{

public:

    Cluster(void):Etot(0.0){}

    void AddHit(const Hit* hit){
        hits.push_back(hit);
        Etot += hit->E;
    }

    std::vector<const Hit*> hits;
    double Etot;

    ClassDef(Cluster,1) // n.b. make sure ROOT_GENERATE_DICTIONARY is in cmake file!
};

#endif // _CLUSTER_H_
