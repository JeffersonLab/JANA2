
#include <iostream>
#include "JContext.h"
#include "Factory.h"


/// Sample data structure containing a Hit
struct Hit {
    double E;
};

/// Sample metadata structure for factories which produce Hits
template <>
struct Metadata<Hit> {
    int count = 0;
};

/// Sample factory which takes a sequence of Hits and "calibrates" it by adding 7
struct CalibratedHitFactory : public BasicFactory<Hit> {

    void process(JContext& event, Metadata<Hit>& metadata, std::vector<Hit*>& output) override {

        auto raw_hits = event.GetVector<Hit>("raw_hits");
        for (auto hit : raw_hits) {
            Hit* calibrated_hit = new Hit(*hit);
            calibrated_hit->E += 7;
            output.push_back(calibrated_hit);
            metadata.count++;
        }
    }
};

/// Demonstration:
/// - Add Factories to the Builder
/// - Create an event Context
/// - Insert data into the event
/// - Retrieve derived data which the Context has computed for us using the appropriate Factory
/// - Retrieve metadata associated with this computation
/// - Handle tags in a clean way

int main() {

    FactoryGeneratorBuilder builder;

    builder.add_factory<Hit>("raw_hits");
    builder.add_basic_factory<Hit, CalibratedHitFactory>("calibrated_hits");

    JInsertableContext event(builder.get_factory_generators());

    std::vector<Hit*> raw_hits;
    for (int i=0; i<20; ++i) {
        Hit* hit = new Hit();
        hit->E = i;
        raw_hits.push_back(hit);
    }

    event.Insert(raw_hits, "raw_hits");

    std::vector<const Hit*> calibrated_hits = event.GetVector<Hit>("calibrated_hits");

    for (const Hit* hit : calibrated_hits) {
        std::cout << hit->E << ", ";
    }
    std::cout << std::endl;
    std::cout << "count: " << event.GetFactory<Hit>("calibrated_hits")->get_metadata().count << std::endl;
}





