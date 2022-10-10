
// Copyright 2022, Jefferson Science Associates, LLC.
// Subject to the terms in the LICENSE file found in the top-level directory.

#include "catch.hpp"
#include <JANA/Utils/JAutoActivator.h>

TEST_CASE("String splitting") {
    auto pair = JAutoActivator::Split("all_just_objname");
    REQUIRE(pair.first == "all_just_objname");
    REQUIRE(pair.second == "");

    pair = JAutoActivator::Split("objname:tag");
    REQUIRE(pair.first == "objname");
    REQUIRE(pair.second == "tag");

    pair = JAutoActivator::Split("name::type:factag");
    REQUIRE(pair.first == "name::type");
    REQUIRE(pair.second == "factag");

    pair = JAutoActivator::Split("name::type");
    REQUIRE(pair.first == "name::type");
    REQUIRE(pair.second == "");

    pair = JAutoActivator::Split("name::type:tag:more_tag");
    REQUIRE(pair.first == "name::type");
    REQUIRE(pair.second == "tag:more_tag");

    pair = JAutoActivator::Split("name::type:tag:more_tag::most_tag");
    REQUIRE(pair.first == "name::type");
    REQUIRE(pair.second == "tag:more_tag::most_tag");

    pair = JAutoActivator::Split("::name::type:tag:more_tag::most_tag");
    REQUIRE(pair.first == "::name::type");
    REQUIRE(pair.second == "tag:more_tag::most_tag");

    pair = JAutoActivator::Split(":I_guess_this_should_be_a_tag_idk");
    REQUIRE(pair.first == "");
    REQUIRE(pair.second == "I_guess_this_should_be_a_tag_idk");
}