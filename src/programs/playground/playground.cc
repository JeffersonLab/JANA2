
#include <iostream>
#include "JContext.h"
#include "Factory.h"

struct JObject{};

struct Hit : public JObject {
    double E;
};

template <>
struct Metadata<Hit> {
    int count = 0;
};


struct CalibratedHitFactory : public FactoryT<Hit> {

    explicit CalibratedHitFactory(std::string tag)
        : FactoryT(std::move(tag)) {}

    void process(JContext& event) override {

        auto raw_hits = event.GetVector<Hit>("raw_hits");
        for (auto hit : raw_hits) {
            Hit* calibrated_hit = new Hit(*hit);
            calibrated_hit->E += 7;
            insert(calibrated_hit);
            ++get_metadata().count;
        }
    }
};


int main() {

    std::vector<FactoryGenerator*> generators;

    generators.push_back(new FactoryGeneratorT<Hit>("raw_hits"));
    generators.push_back(new FactoryGeneratorT<Hit, CalibratedHitFactory>("calibrated_hits"));

    JInsertableContext context(generators);

    std::vector<Hit*> raw_hits;
    for (int i=0; i<20; ++i) {
        Hit* hit = new Hit();
        hit->E = i;
        raw_hits.push_back(hit);
    }
    context.Insert(raw_hits, "raw_hits");

    std::vector<const Hit*> calibrated_hits = context.GetVector<Hit>("calibrated_hits");

    for (const Hit* hit : calibrated_hits) {
        std::cout << hit->E << ", ";
    }
    std::cout << std::endl;
    std::cout << "count: " << context.GetFactory<Hit>("calibrated_hits")->get_metadata().count << std::endl;
}





