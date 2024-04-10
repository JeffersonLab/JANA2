#include <catch.hpp>
#include <iostream>
#include <map>
#include "MultiLevelTopologyTests.h"


enum class Level { None, Timeslice, Event, Subevent };
enum class ComponentType { None, Source, Filter, Map, Split, Merge, Reduce };

std::ostream& operator<<(std::ostream& os, Level l) {
    switch (l) {
        case Level::None:      os << "None"; break;
        case Level::Timeslice: os << "Timeslice"; break;
        case Level::Event:     os << "Event"; break;
        case Level::Subevent:  os << "Subevent"; break;
    }
    return os;
}

std::ostream& operator<<(std::ostream& os, ComponentType ct) {
    switch (ct) {
        case ComponentType::None:    os << "None"; break;
        case ComponentType::Source:  os << "Source"; break;
        case ComponentType::Filter:  os << "Filter"; break;
        case ComponentType::Map:     os << "Map"; break;
        case ComponentType::Split:   os << "Split"; break;
        case ComponentType::Merge:   os << "Merge"; break;
        case ComponentType::Reduce:  os << "Reduce"; break;
    }
    return os;
}


struct Arrow {
    Level level = Level::None;
    ComponentType componentType = ComponentType::None;

    Arrow* input = nullptr;
    Arrow* output = nullptr;

};

class Diagram {
    std::vector<std::string> grid;
    size_t rows, cols, hspace, vspace;

    Diagram(size_t rows, size_t cols, size_t hspace=10, size_t vspace=4) 
        : rows(rows), cols(cols), hspace(hspace), vspace(vspace) {

        for (size_t i=0; i<rows*vspace; ++i) {
            std::string line;
            line.reserve(cols*hspace);
            for (size_t j=0; j<cols*hspace; ++j) {
                line.push_back(' ');
            }
            grid.push_back(std::move(line));
        }
    }

    void add_box(size_t row, size_t col) {
        assert(row < rows);
        assert(col < cols);
        size_t r = row * vspace;
        size_t c = col * hspace;
        grid[r][c] = '[';
        grid[r][c+1] = ']';
    }
/*
    void add_connection(size_t start_row, size_t start_col, size_t end_row, size_t end_col) {
        if (start_row == end_row) {
        }
    }
*/
    void print() {
        for (const auto& line : grid) {
            std::cout << line << std::endl;
        }
    }
};


struct MultiLevelTopologyBuilder {


    std::map<std::pair<Level, ComponentType>, Arrow*> arrows;
    Arrow* source;

    void add(Level l, ComponentType ct) {
        auto arrow = new Arrow;
        arrow->level = l;
        arrow->componentType = ct;
        arrows[{l, ct}] = arrow;
    }

    bool has_connection(Level l1, ComponentType c1, Level l2, ComponentType c2) {

        Arrow* a1 = arrows.at({l1, c1});
        Arrow* a2 = arrows.at({l2, c2});

        bool success = true;
        success &= (a1 != nullptr);
        success &= (a2 != nullptr);
        success &= (a1->output == a2);
        success &= (a2->input == a1);

        return success;
    }

    bool has_no_input(Level l, ComponentType c) {
        return (arrows.at({l, c})->input == nullptr);
    }

    bool has_no_output(Level l, ComponentType c) {
        return (arrows.at({l, c})->output == nullptr);
    }

    void print() {
        Arrow* next = source;
        while (next != nullptr) {
            switch (next->level) {
                case Level::Timeslice: std::cout << "Timeslice:"; break;
                case Level::Event:     std::cout << "  Event:"; break;
                case Level::Subevent:  std::cout << "    Subevent:"; break;
                case Level::None:      std::cout << "None:"; break;
            }
            std::cout << next->componentType << std::endl;
            next = next->output;
        }
    }

    void print_diagram() {
        std::cout << "====================================================================" << std::endl;
        std::cout << "Level       Source    Filter    Map       Split     Merge     Reduce" << std::endl;

        Level levels[] = {Level::Timeslice, Level::Event, Level::Subevent};
        for (int i=0; i<3; ++i) {
            // -----------
            // Print row
            // -----------

            Level l = levels[i];

            switch (l) {
                case Level::None:      std::cout << "None        "; break;
                case Level::Timeslice: std::cout << "Timeslice   "; break;
                case Level::Event:     std::cout << "Event       "; break;
                case Level::Subevent:  std::cout << "Subevent    "; break;
            }

            bool pencil_active = false;
            for (ComponentType ct : {ComponentType::Source, ComponentType::Filter, ComponentType::Map, ComponentType::Split, ComponentType::Merge, ComponentType::Reduce}) {
                auto it = arrows.find({l, ct});

                if (it == arrows.end()) {
                    if (pencil_active) {
                        std::cout << "----------";
                    }
                    else {
                        std::cout << "          ";
                    }
                }
                else {
                    std::cout << "[]";
                    pencil_active = (it->second->output != nullptr && it->second->output->level == l);
                    if (pencil_active) {
                        std::cout << "--------";
                    }
                    else {
                        std::cout << "        ";
                    }
                }
            }
            std::cout << std::endl;

            // --------
            // Print connectors to lower level
            // --------
            // Skip lowest level
            if (i < 2) {
                // Vertical connector to split and merge
                auto split_it = arrows.find({l, ComponentType::Split});
                if (split_it != arrows.end()) {
                    std::cout << "                                          |";
                }
                else {
                    std::cout << "                                           ";
                }
                auto merge_it = arrows.find({l, ComponentType::Merge});
                if (merge_it != arrows.end()) {
                    std::cout << "         |";
                }
                else {
                    std::cout << "          ";
                }
                std::cout << std::endl;

                // Horizontal connector
                std::cout << "            ";
                bool pencil_active = false;
                for (ComponentType ct : {ComponentType::Source, ComponentType::Filter, ComponentType::Map, ComponentType::Split, ComponentType::Merge, ComponentType::Reduce}) {

                    bool found_split = (split_it != arrows.end() && ct == ComponentType::Split);
                    bool found_splitee = (split_it != arrows.end() && split_it->second->output->componentType == ct);
                    bool found_merge = (merge_it != arrows.end() && ct == ComponentType::Merge);
                    bool found_mergee = (merge_it != arrows.end() && merge_it->second->input->componentType == ct);

                    if (found_splitee) {
                        pencil_active = !pencil_active;
                    }
                    if (found_split) {
                        pencil_active = !pencil_active;
                    }
                    if (found_merge) {
                        pencil_active = !pencil_active;
                    }
                    if (found_mergee) {
                        pencil_active = !pencil_active;
                    }
                    if (pencil_active) {
                        std::cout << "----------";
                    }
                    else {
                        std::cout << "          ";
                    }
                }
                std::cout << std::endl;


                // Vertical connector to split and merge joinees
                std::cout << "            ";
                for (ComponentType ct : {ComponentType::Source, ComponentType::Filter, ComponentType::Map, ComponentType::Split, ComponentType::Merge, ComponentType::Reduce}) {
                    if (split_it != arrows.end() && split_it->second->output->componentType == ct) {
                        std::cout << "|         ";
                    }
                    else if (merge_it != arrows.end() && merge_it->second->input->componentType == ct) {
                        std::cout << "|         ";
                    }
                    else {
                        std::cout << "          ";
                    }
                }
                std::cout << std::endl;
            }
        }
        std::cout << "====================================================================" << std::endl;
    }


    void wire_and_print() {

        source = nullptr;
        for (Level l : {Level::Timeslice, Level::Event, Level::Subevent} ) {
            Arrow* last = nullptr;

            for (ComponentType ct : {ComponentType::Source, ComponentType::Filter, ComponentType::Map, ComponentType::Split, ComponentType::Merge, ComponentType::Reduce}) {
                auto it = arrows.find({l, ct});
                if (it != arrows.end()) {

                    Arrow* next = it->second;
                    if (last != nullptr) {
                        next->input = last;
                        last->output = next;
                    }
                    last = next;

                    // Handle sources:
                    if (ct == ComponentType::Source) {
                        if (source != nullptr) {
                            throw std::runtime_error("Too many sources!");
                        }
                        source = next;
                    }
                }
            }
        }

        Level levels[] = {Level::Timeslice, Level::Event, Level::Subevent};
        for (int i=0; i<2; ++i) {
            Level upper = levels[i];
            Level lower = levels[i+1];

            auto split_it = arrows.find({upper, ComponentType::Split});
            if (split_it != arrows.end()) {
                // Have splitter, so we need to find the lower-level arrow it connects to
                bool found_joinee = false;
                for (ComponentType ct : {ComponentType::Filter, ComponentType::Map, ComponentType::Split, ComponentType::Reduce}) {
                    auto it = arrows.find({lower, ct});
                    if (it != arrows.end()) {
                        Arrow* joinee = it->second;
                        split_it->second->output = joinee;
                        joinee->input = split_it->second;
                        found_joinee = true;
                        break;
                    }
                }
                if (!found_joinee) {
                    throw std::runtime_error("Invalid topology: Unable to find successor to split");
                }
            }

            auto merge_it = arrows.find({upper, ComponentType::Merge});
            if (merge_it != arrows.end()) {
                // Have merger, so we need to find the lower-level arrow it connects to
                bool found_joinee = false;
                for (ComponentType ct : {ComponentType::Reduce, ComponentType::Merge, ComponentType::Map, ComponentType::Filter}) {
                    auto it = arrows.find({lower, ct});
                    if (it != arrows.end()) {
                        Arrow* joinee = it->second;
                        merge_it->second->input = joinee;
                        joinee->output = merge_it->second;
                        found_joinee = true;
                        break;
                    }
                }
                if (!found_joinee) {
                    throw std::runtime_error("Invalid topology: Unable to find predecessor to merge");
                }
            }
        }

        print();
    }
};



TEST_CASE("MultiLevelTopologyBuilderTests") {
    MultiLevelTopologyBuilder b;

    SECTION("Default topology") {

        b.add(Level::Event, ComponentType::Source);
        b.add(Level::Event, ComponentType::Map);

        b.wire_and_print();

        REQUIRE(b.has_no_input( Level::Event, ComponentType::Source ));
        REQUIRE(b.has_connection( Level::Event, ComponentType::Source, Level::Event, ComponentType::Map ));
        REQUIRE(b.has_no_output( Level::Event, ComponentType::Map ));
    }

    SECTION("Default with separated map/reduce phases") {

        b.add(Level::Event, ComponentType::Source);
        b.add(Level::Event, ComponentType::Map);
        b.add(Level::Event, ComponentType::Reduce);

        b.wire_and_print();

        REQUIRE(b.has_no_input( Level::Event, ComponentType::Source ));
        REQUIRE(b.has_connection( Level::Event, ComponentType::Source, Level::Event, ComponentType::Map ));
        REQUIRE(b.has_connection( Level::Event, ComponentType::Map, Level::Event, ComponentType::Reduce ));
        REQUIRE(b.has_no_output( Level::Event, ComponentType::Reduce ));
    }

    SECTION("Timeslices topology") {

        b.add(Level::Timeslice, ComponentType::Source);
        b.add(Level::Timeslice, ComponentType::Map);
        b.add(Level::Timeslice, ComponentType::Split);
        b.add(Level::Timeslice, ComponentType::Merge);
        b.add(Level::Event, ComponentType::Map);
        b.add(Level::Event, ComponentType::Reduce);

        b.wire_and_print();

        REQUIRE(b.has_no_input( Level::Timeslice, ComponentType::Source ));
        REQUIRE(b.has_connection( Level::Timeslice, ComponentType::Source, Level::Timeslice, ComponentType::Map ));
        REQUIRE(b.has_connection( Level::Timeslice, ComponentType::Map, Level::Timeslice, ComponentType::Split ));
        REQUIRE(b.has_no_output( Level::Timeslice, ComponentType::Merge ));

        REQUIRE(b.has_connection( Level::Event, ComponentType::Map, Level::Event, ComponentType::Reduce ));
        REQUIRE(b.has_connection( Level::Timeslice, ComponentType::Split, Level::Event, ComponentType::Map ));
        REQUIRE(b.has_connection( Level::Event, ComponentType::Reduce, Level::Timeslice, ComponentType::Merge ));

    }

    SECTION("Three-level topology") {

        b.add(Level::Timeslice, ComponentType::Source);
        b.add(Level::Timeslice, ComponentType::Map);
        b.add(Level::Timeslice, ComponentType::Split);
        b.add(Level::Timeslice, ComponentType::Merge);
        b.add(Level::Event, ComponentType::Map);
        b.add(Level::Event, ComponentType::Split);
        b.add(Level::Event, ComponentType::Merge);
        b.add(Level::Event, ComponentType::Reduce);
        b.add(Level::Subevent, ComponentType::Map);

        b.wire_and_print();

        REQUIRE(b.has_no_input( Level::Timeslice, ComponentType::Source ));
        REQUIRE(b.has_connection( Level::Timeslice, ComponentType::Source, Level::Timeslice, ComponentType::Map ));
        REQUIRE(b.has_connection( Level::Timeslice, ComponentType::Map, Level::Timeslice, ComponentType::Split ));
        REQUIRE(b.has_connection( Level::Timeslice, ComponentType::Split, Level::Event, ComponentType::Map ));
        REQUIRE(b.has_connection( Level::Event, ComponentType::Map, Level::Event, ComponentType::Split ));
        REQUIRE(b.has_connection( Level::Event, ComponentType::Split, Level::Subevent, ComponentType::Map ));
        REQUIRE(b.has_connection( Level::Subevent, ComponentType::Map, Level::Event, ComponentType::Merge ));
        REQUIRE(b.has_connection( Level::Event, ComponentType::Merge, Level::Event, ComponentType::Reduce ));
        REQUIRE(b.has_connection( Level::Event, ComponentType::Reduce, Level::Timeslice, ComponentType::Merge ));
        REQUIRE(b.has_no_output( Level::Timeslice, ComponentType::Merge ));

    }

    SECTION("Too many sources") {
        b.add(Level::Timeslice, ComponentType::Source);
        b.add(Level::Timeslice, ComponentType::Map);
        b.add(Level::Timeslice, ComponentType::Split);
        b.add(Level::Timeslice, ComponentType::Merge);
        b.add(Level::Event, ComponentType::Map);
        b.add(Level::Event, ComponentType::Reduce);
        b.add(Level::Event, ComponentType::Source);

        REQUIRE_THROWS(b.wire_and_print());
    }

    SECTION("Inactive components") {
        b.add(Level::Timeslice, ComponentType::Map);
        b.add(Level::Timeslice, ComponentType::Split);
        b.add(Level::Timeslice, ComponentType::Merge);
        b.add(Level::Event, ComponentType::Map);
        b.add(Level::Event, ComponentType::Reduce);
        b.add(Level::Event, ComponentType::Source);

        b.wire_and_print();
        // Timeslice-level components are deactivated
    }
}




TEST_CASE("TimeslicesTests") {

    auto parms = new JParameterManager;
    parms->SetParameter("log:trace", "JScheduler,JArrow,JArrowProcessingController");
    parms->SetParameter("jana:nevents", "5");
    JApplication app(parms);
    
    app.Add(new MyTimesliceSource);
    app.Add(new MyTimesliceUnfolder);
    app.Add(new MyEventProcessor);
    app.Add(new JFactoryGeneratorT<MyProtoClusterFactory>);
    app.Add(new JFactoryGeneratorT<MyClusterFactory>);
    app.SetTicker(true);
    app.Run();
}


} // namespace timeslice_tests
} // namespce jana






