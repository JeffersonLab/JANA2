
#include <iostream>
#include <JANA/JObject.h>
#include "JEvent2.h"

struct Hit : JObject {
    double Etot, x, y, t;
};


int main() {

    Hit* hit = new Hit();
    hit->Etot = 0.5;
    hit->x = 3.3;
    hit->y = 7.3;
    hit->t = 99.9;

    JEventMutable jemv;
    jemv.Insert(hit);
    auto item = jemv.GetSingle<Hit>();

    JEvent* jep = &jemv;
    //auto item2 = jep->GetSingle<Hit>();
    //std::cout << item2->Etot << std::endl;

}





