
#include <catch.hpp>
#include <JANA/Engine/JJunctionArrow.h>

namespace jana {
namespace arrowtests {


struct TestMapArrow : public JJunctionArrow<TestMapArrow, int, double> {

    TestMapArrow(JMailbox<int*>* qi,
                 JPool<int>* pi,
                 JPool<double>* pd,
                 JMailbox<double*>* qd) 
    : JJunctionArrow<TestMapArrow,int,double>("testmaparrow", false, false, true) {

        first_input = {this, qi, true, 1, 1};
        first_output = {this, pi, false, 1, 1};
        second_input = {this, pd, true, 1, 1};
        second_output = {this, qd, false, 1, 1};
    }

    Status process(Data<int>& input_int, 
                   Data<int>& output_int, 
                   Data<double>& input_double, 
                   Data<double>& output_double) {
        std::cout << "Hello from process" << std::endl;

        REQUIRE(input_int.item_count == 1);
        REQUIRE(input_int.reserve_count == 1);
        REQUIRE(output_int.item_count == 0);
        REQUIRE(output_int.reserve_count == 0);
        REQUIRE(input_double.item_count == 1);
        REQUIRE(input_double.reserve_count == 0);
        REQUIRE(output_double.item_count == 0);
        REQUIRE(output_double.reserve_count == 1);
        
        int* x = input_int.items[0];
        input_int.items[0] = nullptr;
        input_int.item_count = 0;

        // TODO: Maybe user shouldn't be allowed to modify reserve_count at all 
        // TODO Maybe user should only be allowed to push and pull from ... range...?
        
        double* y = input_double.items[0];
        input_double.items[0] = nullptr;
        input_double.item_count = 0;

        // Do something useful here
        *y = *x + 22.2;
        
        output_int.items[0] = x;
        output_int.item_count = 1;

        output_double.items[0] = y;
        output_double.item_count = 1;
        return Status::KeepGoing;
    }

};


TEST_CASE("ArrowTests_Basic") {

    JMailbox<int*> qi {2, 1, false};
    JPool<int> pi {5, 1, true};
    JPool<double> pd {5, 1, true};
    JMailbox<double*> qd {2, 1, false};

    pi.init();
    pd.init();

    TestMapArrow a {&qi, &pi, &pd, &qd};

    int* x;
    pi.pop(&x, 1, 1, 0);
    *x = 100;

    qi.push_and_unreserve(&x, 1, 0, 0);
    JArrowMetrics m;
    a.execute(m, 0);

    double* y;
    qd.pop_and_reserve(&y, 1, 1, 0);
    REQUIRE(*y == 122.2);

}

    
} // namespace arrowtests
} // namespace jana




