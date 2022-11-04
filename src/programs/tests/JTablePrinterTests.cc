
// Copyright 2022, Jefferson Science Associates, LLC.
// Subject to the terms in the LICENSE file found in the top-level directory.

#include <catch.hpp>
#include <JANA/Utils/JTablePrinter.h>

TEST_CASE("Cell split by newlines") {
    auto result = JTablePrinter::SplitContents("This\nis a\ntest", 10);
    REQUIRE(result.size() == 3);
    REQUIRE(result[0] == "This");
    REQUIRE(result[1] == "is a");
    REQUIRE(result[2] == "test");
}

TEST_CASE("Cell split by spaces trivial") {
    auto result = JTablePrinter::SplitContents("This is a test", 20);
    REQUIRE(result[0] == "This is a test");
    REQUIRE(result.size() == 1);
}

TEST_CASE("Cell split by newlines and spaces trivial") {
    auto result = JTablePrinter::SplitContents("This is a \ntest", 20);
    REQUIRE(result[0] == "This is a ");
    REQUIRE(result[1] == "test");
    REQUIRE(result.size() == 2);
}

TEST_CASE("Cell split by spaces") {
    auto result = JTablePrinter::SplitContents("This is a test", 6);
    for (auto& s: result) {
        std::cout << ">> " << s << std::endl;
    }
    REQUIRE(result.size() == 3);
    REQUIRE(result[0] == "This");
    REQUIRE(result[1] == "is a");
    REQUIRE(result[2] == "test");
}

TEST_CASE("Cell split by multiple spaces and newline") {
    auto result = JTablePrinter::SplitContents("This is  \n        a test", 10);
    for (auto& s: result) {
        std::cout << ">> " << s << std::endl;
    }
    REQUIRE(result.size() == 3);
    REQUIRE(result[0] == "This is  ");
    REQUIRE(result[1] == "        a");
    REQUIRE(result[2] == "test");
}

TEST_CASE("Nothing splittable") {
    auto result = JTablePrinter::SplitContents("This_is_a_test", 5);
    for (auto& s: result) {
        std::cout << ">> " << s << std::endl;
    }
    REQUIRE(result.size() == 3);
    REQUIRE(result[0] == "This_");
    REQUIRE(result[1] == "is_a_");
    REQUIRE(result[2] == "test");

}

TEST_CASE("Evenly divides") {
    auto result = JTablePrinter::SplitContents("aaa bbb ccc", 3);
    for (auto& s: result) {
        std::cout << ">> " << s << std::endl;
    }
    REQUIRE(result.size() == 3);
    REQUIRE(result[0] == "aaa");
    REQUIRE(result[1] == "bbb");
    REQUIRE(result[2] == "ccc");

}