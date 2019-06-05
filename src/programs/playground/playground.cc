
#include <iostream>
#include "JContext.h"
#include "Factory.h"

struct JObject{};

struct DummyData : public JObject {
    double x = 22.2;
};

struct DummyFactory : public FactoryT<DummyData> {
    double y = 44;
};


int main() {

    std::vector<FactoryGenerator*> generators;
    generators.push_back(new FactoryGeneratorT<DummyFactory>());

    JContext context(generators);


    auto fac = context.GetFactory<DummyData>(); // returns FactoryT<DummyData>, not DummyFactory
    // This is (supposedly) a feature: we don't _want_ to know which Factory we used!
    // If we did know exactly which factory was used, we would be breaking the decoupling!
    // Instead, we could be creating Factories through template specialization instead of inheritance

}





