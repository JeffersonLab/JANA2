
// Copyright 2023, Jefferson Science Associates, LLC.
// Subject to the terms in the LICENSE file found in the top-level directory.
#include <iostream>
#include <podio/CollectionBase.h>
#include "datamodel/MutableExampleHit.h"
#include "datamodel/ExampleHitCollection.h"

int main() {
    std::cout << "Hello world" << std::endl;
    auto coll = new ExampleHitCollection;
    MutableExampleHit hit;
    hit.cellID(22);
    hit.energy(100.0);
    hit.x(0);
    hit.y(0);
    hit.z(0);
    coll->push_back(hit);

}