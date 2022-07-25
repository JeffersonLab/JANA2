
// Copyright 2020, Jefferson Science Associates, LLC.
// Subject to the terms in the LICENSE file found in the top-level directory.


#include <JANA/Services/JParameterManager.h>
#include "catch.hpp"

TEST_CASE("JParameterManager::SetDefaultParameter") {

    JParameterManager jpm;


    SECTION("Multiple calls to SetDefaultParameter with same defaults succeed") {

        jpm.SetParameter("testing:dummy_var", 22);

        int x = 44;
        jpm.SetDefaultParameter("testing:dummy_var", x);
        REQUIRE(x == 22);

        int y = 44;
        jpm.SetDefaultParameter("testing:dummy_var", y);
        REQUIRE(y == 22);
    }


    SECTION("Multiple calls to SetDefaultParameter with different defaults") {

        // If set, the user provided value overrides ALL default values

        jpm.SetParameter("testing:dummy_var", 22);

        int x = 44;
        jpm.SetDefaultParameter("testing:dummy_var", x);
        REQUIRE(x == 22);

        int y = 77;
        jpm.SetDefaultParameter("testing:dummy_var", y);
        REQUIRE(x == 22);


        // If unset, the _first_ default value wins
        // TODO: This is not right. We can do better

        int z = 44;
        jpm.SetDefaultParameter("testing:dummy_var_2", z);
        REQUIRE(z == 44);

        int zz = 77;
        jpm.SetDefaultParameter("testing:dummy_var_2", zz);
        REQUIRE(zz == 44); // Ideally 77
    }
}


TEST_CASE("JParameterManagerBoolTests") {
    JParameterManager jpm;

    SECTION("'0' parses to false") {
        jpm.SetParameter("test_param", "0");
        bool val = jpm.GetParameterValue<bool>("test_param");
        REQUIRE(val == false);
    }

    SECTION("'1' parses to true") {
        jpm.SetParameter("test_param", "1");
        bool val = jpm.GetParameterValue<bool>("test_param");
        REQUIRE(val == true);
    }

    SECTION("'off' parses to false") {
        jpm.SetParameter("test_param", "off");
        bool val = jpm.GetParameterValue<bool>("test_param");
        REQUIRE(val == false);
    }

    SECTION("'on' parses to true") {
        jpm.SetParameter("test_param", "on");
        bool val = jpm.GetParameterValue<bool>("test_param");
        REQUIRE(val == true);
    }

    SECTION("'true' parses to true") {
        jpm.SetParameter("test_param", "true");
        bool val = jpm.GetParameterValue<bool>("test_param");
        REQUIRE(val == true);
    }

    SECTION("'false' parses to false") {
        jpm.SetParameter("test_param", "false");
        bool val = jpm.GetParameterValue<bool>("test_param");
        REQUIRE(val == false);
    }

    SECTION("Parsing anything else as bool throws an exception") {
        jpm.SetParameter("test_param", "maybe");
        CHECK_THROWS(jpm.GetParameterValue<bool>("test_param"));
    }

    SECTION("Stringify still works") {
        jpm.SetParameter("test_param", false);
        std::string val = jpm.GetParameterValue<std::string>("test_param");
        REQUIRE(val == "0");

        jpm.SetParameter("test_param", true);
        val = jpm.GetParameterValue<std::string>("test_param");
        REQUIRE(val == "1");
    }
}

TEST_CASE("JParameterManager_VectorParams") {
    JParameterManager jpm;

    SECTION("Reading a vector of strings") {
        jpm.SetParameter("test", "simple,whitespace in middle, also with whitespace padding ");
        std::vector<std::string> vals;
        jpm.GetParameter<std::vector<std::string>>("test", vals);

        REQUIRE(vals[0] == "simple");
        REQUIRE(vals[1] == "whitespace in middle");
        REQUIRE(vals[2] == " also with whitespace padding ");
    }
    SECTION("Writing a vector of strings") {
        std::vector<std::string> inputs;
        inputs.emplace_back("first");
        inputs.emplace_back("second one");
        inputs.emplace_back(" third one ");

        jpm.SetDefaultParameter("test", inputs);
        std::vector<std::string> outputs;
        auto param = jpm.GetParameter("test", outputs);
        REQUIRE(param->GetValue() == "first,second one, third one ");
    }
    SECTION("Reading a vector of ints") {
        jpm.SetParameter("test", "1,2, 3 ");
        std::vector<int32_t> vals;
        jpm.GetParameter("test", vals);

        REQUIRE(vals[0] == 1);
        REQUIRE(vals[1] == 2);
        REQUIRE(vals[2] == 3);
    }
    SECTION("Writing a vector of ints") {
        std::vector<int32_t> inputs;
        inputs.emplace_back(22);
        inputs.emplace_back(49);
        inputs.emplace_back(42);

        jpm.SetDefaultParameter("test", inputs);
        std::vector<std::string> outputs;
        auto param = jpm.GetParameter("test", outputs);
        REQUIRE(param->GetValue() == "22,49,42");
    }
    SECTION("Reading a vector of floats") {
        jpm.SetParameter("test", "1,2,3");
        std::vector<float> vals;
        jpm.GetParameter("test", vals);

        REQUIRE(vals[0] == 1.0f);
        REQUIRE(vals[1] == 2.0f);
        REQUIRE(vals[2] == 3.0f);
    }
    SECTION("Writing a vector of floats") {
        std::vector<float> inputs;
        inputs.emplace_back(22.0);
        inputs.emplace_back(49.2);
        inputs.emplace_back(42.0);

        jpm.SetDefaultParameter("test", inputs);
        std::vector<float> outputs;
        auto param = jpm.GetParameter("test", outputs);
        REQUIRE(param->GetValue() == "22,49.2,42");
    }

}



