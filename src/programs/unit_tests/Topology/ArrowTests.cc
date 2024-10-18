
#include <catch.hpp>
#include <JANA/Topology/JJunctionArrow.h>
#include <JANA/JEvent.h>

namespace jana {
namespace arrowtests {


struct ArrowTestData {
    int x;
    double y;
};

using EventT = std::shared_ptr<JEvent>;
struct TestJunctionArrow : public JJunctionArrow<TestJunctionArrow> {

    TestJunctionArrow(JMailbox<EventT*>* qi,
                      JEventPool* pi,
                      JEventPool* pd,
                      JMailbox<EventT*>* qd) 
    : JJunctionArrow("testjunctionarrow", false, false, true) {

        first_input.set_queue(qi);
        first_output.set_pool(pi);
        second_input.set_pool(pd);
        second_output.set_queue(qd);
    }

    Status process(Data& input_int, 
                   Data& output_int, 
                   Data& input_double, 
                   Data& output_double) {
        std::cout << "Hello from process" << std::endl;

        REQUIRE(input_int.item_count == 1);
        REQUIRE(input_int.reserve_count == 1);
        REQUIRE(output_int.item_count == 0);
        REQUIRE(output_int.reserve_count == 0);
        REQUIRE(input_double.item_count == 1);
        REQUIRE(input_double.reserve_count == 0);
        REQUIRE(output_double.item_count == 0);
        REQUIRE(output_double.reserve_count == 1);
        
        EventT* x_event = input_int.items[0];
        input_int.items[0] = nullptr;
        input_int.item_count = 0;

        // TODO: Maybe user shouldn't be allowed to modify reserve_count at all 
        // TODO Maybe user should only be allowed to push and pull from ... range...?
        
        EventT* y_event = input_double.items[0];
        input_double.items[0] = nullptr;
        input_double.item_count = 0;

        auto data = (*x_event)->Get<ArrowTestData>();
        int x = data.at(0)->x;
        // Do something useful here
        double y = x + 22.2;
        (*y_event)->Insert(new ArrowTestData{.x = x, .y = y});
        
        output_int.items[0] = x_event;
        output_int.item_count = 1;

        output_double.items[0] = y_event;
        output_double.item_count = 1;
        return Status::KeepGoing;
    }

};


TEST_CASE("ArrowTests_Basic") { 

    JApplication app;
    app.Initialize();
    auto jcm = app.GetService<JComponentManager>();

    JMailbox<EventT*> qi {2, 1, false};
    JEventPool pi {jcm, 5, 1, true};
    JEventPool pd {jcm, 5, 1, true};
    JMailbox<EventT*> qd {2, 1, false};

    TestJunctionArrow a {&qi, &pi, &pd, &qd};

    EventT* x = nullptr;
    pi.pop(&x, 1, 1, 0);
    REQUIRE(x != nullptr);
    REQUIRE((*x)->GetEventNumber() == 0);
    (*x)->Insert(new ArrowTestData {.x = 100, .y=0});

    qi.push_and_unreserve(&x, 1, 0, 0);
    JArrowMetrics m;
    a.execute(m, 0);

    EventT* y;
    qd.pop_and_reserve(&y, 1, 1, 0);
    auto data = (*y)->Get<ArrowTestData>();
    REQUIRE(data.at(0)->y == 122.2);

}

    
} // namespace arrowtests
} // namespace jana




