
// Copyright 2020, Jefferson Science Associates, LLC.
// Subject to the terms in the LICENSE file found in the top-level directory.


#include "catch.hpp"

#include <JANA/JEvent.h>
#include "JEventTests.h"


TEST_CASE("JEventInsertTests") {


    auto event = std::make_shared<JEvent>();
    event->SetFactorySet(new JFactorySet);

    SECTION("Single-item JEvent::Insert() can be retrieved via JEvent::Get()") {
        auto input = new FakeJObject(22);
        event->Insert(input);
        auto output = event->GetSingle<FakeJObject>();
        REQUIRE(output->datum == input->datum);

        const FakeJObject* alternative;
        event->Get(&alternative);
        REQUIRE(output->datum == alternative->datum);

    }

    SECTION("Multi-item JEvent::Insert() can be retrieved via JEvent::Get()") {
        std::vector<FakeJObject*> input;          // const is optional for input
        input.push_back(new FakeJObject(1));
        input.push_back(new FakeJObject(2));
        input.push_back(new FakeJObject(3));
        event->Insert(input);

        std::vector<const FakeJObject*> output;   // const is required for output
        event->Get(output);

        REQUIRE(output.size() == input.size());
        for (size_t i=0; i<output.size(); ++i) {
            REQUIRE(output[i]->datum == input[i]->datum);
        }
    }

    SECTION("JEvent::Insert() respects both tag and typeid") {
        event->Insert(new FakeJObject(22), "first_tag");
        event->Insert(new FakeJObject(49), "second_tag");
        event->Insert(new DifferentFakeJObject(7.6), "first_tag");

        std::vector<const FakeJObject*> output;

        event->Get(output, "first_tag");
        REQUIRE(output.size() == 1);
        REQUIRE(output[0]->datum == 22);

        output.clear();
        event->Get(output, "second_tag");
        REQUIRE(output.size() == 1);
        REQUIRE(output[0]->datum == 49);

        std::vector<const DifferentFakeJObject*> different_output;

        event->Get(different_output, "first_tag");
        REQUIRE(different_output.size() == 1);
        REQUIRE(different_output[0]->sample == 7.6);
    }

    SECTION("Repeated calls to JEvent::Insert() aggregate") {

        event->Insert(new FakeJObject(22), "first_tag");
        event->Insert(new FakeJObject(49), "first_tag");
        event->Insert(new FakeJObject(618), "first_tag");

        std::vector<const FakeJObject*> output;
        event->Get(output, "first_tag");
        REQUIRE(output.size() == 3);
        REQUIRE(output[0]->datum == 22);
        REQUIRE(output[1]->datum == 49);
        REQUIRE(output[2]->datum == 618);
    }

    SECTION("Inserted JObjects get deleted when enclosing JEvent does") {
        FakeJObject* obj = new FakeJObject(618);
        bool deleted = false;
        obj->deleted = &deleted;
        JEvent* event_ptr = new JEvent();
        event_ptr->SetFactorySet(new JFactorySet);
        event_ptr->Insert(obj, "tag");
        REQUIRE(deleted == false);
        delete event_ptr;
        REQUIRE(deleted == true);
    }


    SECTION("JEvent::GetFactory<T> handles missing factories correctly") {

        // When present, GetFactory<T> returns a pointer to the correct factory (dummy or otherwise)
        event->Insert(new FakeJObject(22), "present_tag");
        auto present_factory = event->GetFactory<FakeJObject>("present_tag");
        REQUIRE(present_factory != nullptr);

        // By default, return nullptr for compatibility with older code
        auto missing_factory = event->GetFactory<FakeJObject>("absent_tag");
        REQUIRE(missing_factory == nullptr);

        // This is effected via the throw_on_missing parameter
        missing_factory = event->GetFactory<FakeJObject>("absent_tag", false);
        REQUIRE(missing_factory == nullptr);

        // GetFactory<T> can conveniently throw an exception if factory is missing.
        // This is useful when said data is required
        REQUIRE_THROWS(event->GetFactory<FakeJObject>("absent_tag", true));
    }

    // -----------------
    // C-style GetSingle
    // -----------------
	SECTION("JEvent::Get(Single) updates destination to null when factory is present but empty") {
		std::vector<FakeJObject*> objects;
		event->Insert(objects);
		const FakeJObject* result;
		JFactory* factory = event->Get(&result);
		REQUIRE(result == nullptr);
		REQUIRE(factory != nullptr);
	}

	SECTION("JEvent::Get(Single) throws an exception when factory is missing") {
    	const FakeJObject* result;
		REQUIRE_THROWS(event->Get(&result));
	}

	SECTION("JEvent::Get(Single) updates destination to point to the only object when the factory contains only one object") {
		auto inserted = new FakeJObject(22);
		event->Insert(inserted);
		const FakeJObject* result;
		auto factory = event->Get(&result);
		REQUIRE(result->datum == inserted->datum);      // Same contents
		REQUIRE(result == inserted);                    // Same pointer
		REQUIRE(factory != nullptr);                    // Factory exists
	}

	SECTION("JEvent::Get(Single) returns the first object when the factory contains multiple objects") {
		auto first = new FakeJObject(22);
		event->Insert(first);
		event->Insert(new FakeJObject(99));
		event->Insert(new FakeJObject(42));
		const FakeJObject* result;
		auto factory = event->Get(&result);
		REQUIRE(result->datum == first->datum);      // Same contents
		REQUIRE(result == first);                    // Same pointer
		REQUIRE(factory != nullptr);                    // Factory exists
	}

		// ---------
		// GetSingle
	// ---------

    SECTION("JEvent::GetSingle returns null when factory is present but empty") {
    	std::vector<FakeJObject*> objects;
    	event->Insert(objects);
    	auto object = event->GetSingle<FakeJObject>();
    	REQUIRE(object == nullptr);
    }

	SECTION("JEvent::GetSingle throws an exception when factory is missing") {
		REQUIRE_THROWS(event->GetSingle<FakeJObject>());
	}

	SECTION("JEvent::GetSingle returns the only object when the factory contains only one object") {
    	auto inserted = new FakeJObject(22);
		event->Insert(inserted);
		auto retrieved = event->GetSingle<FakeJObject>();
		REQUIRE(retrieved->datum == inserted->datum);      // Same contents
		REQUIRE(retrieved == inserted);                    // Same pointer
	}

	SECTION("JEvent::GetSingle returns the first object when the factory contains multiple objects") {
		auto first = new FakeJObject(22);
		event->Insert(first);
		event->Insert(new FakeJObject(99));
		event->Insert(new FakeJObject(42));
		auto retrieved = event->GetSingle<FakeJObject>();
		REQUIRE(retrieved->datum == first->datum);      // Same contents
		REQUIRE(retrieved == first);                    // Same pointer
	}

	// ---------------
	// GetSingleStrict
	// ---------------

	SECTION("JEvent::GetSingleStrict throws an exception when factory is present but empty") {
		std::vector<FakeJObject*> objects; // empty
		event->Insert(objects);  // creates an empty dummy factory
		REQUIRE_THROWS(event->GetSingleStrict<FakeJObject>());
	}

	SECTION("JEvent::GetSingleStrict throws an exception when factory is missing") {
		REQUIRE_THROWS(event->GetSingleStrict<FakeJObject>());
	}

	SECTION("JEvent::GetSingleStrict returns the only object when the factory contains only one object") {
		auto inserted = new FakeJObject(22);
		event->Insert(inserted);
		auto retrieved = event->GetSingle<FakeJObject>();
		REQUIRE(retrieved->datum == inserted->datum);      // Same contents
		REQUIRE(retrieved == inserted);                    // Same pointer
	}

	SECTION("JEvent::GetSingleStrict throws an exception when the factory contains multiple objects") {
		event->Insert(new FakeJObject(22));
		event->Insert(new FakeJObject(99));
		event->Insert(new FakeJObject(42));
		REQUIRE_THROWS(event->GetSingleStrict<FakeJObject>());
	}

}

