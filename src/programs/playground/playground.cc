
#include <iostream>
#include "JContext.h"
#include "Factory.h"

struct JObject{};

struct Hit : public JObject {
    double E;
};

template <>
struct Metadata<Hit> {
    int count = 22;
};

template <>
void FactoryT<Hit>::process(JContext& c) {
    std::cout << "Calling default process() for " << get_type_name() << std::endl;
    std::cout << "Reading Metadata<Hit> = " << get_metadata().count++ << std::endl;
}


struct WeirdHitFactory : public FactoryT<Hit> {

    explicit WeirdHitFactory(std::string tag)
        : FactoryT(std::move(tag)) {}

    void process(JContext& c) {
        std::cout << "Calling weird process() from " << get_type_name() << std::endl;
        std::cout << "Reading Metadata<Hit> = " << get_metadata().count++ << std::endl;
    }
};


int main() {

    std::vector<FactoryGenerator*> generators;

    //generators.push_back(new FactoryGeneratorT<Hit>(""));   // Uses default FactoryT<T> impl
    generators.push_back(new FactoryGeneratorT<Hit, WeirdHitFactory>(""));  // Uses arbitrary impl

    JContext context(generators);

    auto fac = context.GetFactory<Hit>();
    // Returns FactoryT<DummyData>, not DummyFactory
    // This is not a bug: we don't _want_ to know which Factory we used!
    // If we did know exactly which factory was used, plugins would no longer be substitutable!

    fac->process(context);

    std::cout << "process() count: " << fac->get_metadata().count << std::endl;

}





