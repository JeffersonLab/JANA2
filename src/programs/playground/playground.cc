
#include <iostream>
#include "JContext.h"
#include "Factory.h"

struct JObject{};

struct Hit : public JObject {
    double x;
};

template <>
struct FactoryT<Hit>::Metadata {
    int x = 1000;
    double y = 22;
};

struct DummyFactory : public FactoryT<Hit> {
    DummyFactory(std::string tag) : FactoryT(tag, new FactoryT<Hit>::Metadata()) {}
};


int main() {

    std::vector<FactoryGenerator*> generators;
    generators.push_back(new FactoryGeneratorT<DummyFactory, Hit>(""));

    JContext context(generators);

    auto fac = context.GetFactory<Hit>();
    // Returns FactoryT<DummyData>, not DummyFactory
    // This is not a bug: we don't _want_ to know which Factory we used!
    // If we did know exactly which factory was used, plugins would no longer be substitutable!

    auto metadata = fac->get_metadata();
    std::cout << "Extracted metadata: " << metadata->y << std::endl;

}





