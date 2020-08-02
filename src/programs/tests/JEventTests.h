
// Copyright 2020, Jefferson Science Associates, LLC.
// Subject to the terms in the LICENSE file found in the top-level directory.


#ifndef JANA2_JEVENTTESTS_H
#define JANA2_JEVENTTESTS_H

#include <JANA/JObject.h>
#include <iostream>

struct FakeJObject : public JObject {
    int datum;
    bool* deleted = nullptr;
    FakeJObject(int datum) : datum(datum) {};
    ~FakeJObject() {
        if (deleted != nullptr) {
            *deleted = true;
        }
    }
};

struct DifferentFakeJObject : public JObject {
    double sample;
    DifferentFakeJObject(double sample) : sample(sample) {};
};



#endif //JANA2_JEVENTTESTS_H
