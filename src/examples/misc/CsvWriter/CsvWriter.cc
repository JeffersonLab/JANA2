
// Copyright 2020, Jefferson Science Associates, LLC.
// Subject to the terms in the LICENSE file found in the top-level directory.


#include <JANA/JEventProcessor.h>
#include <JANA/JObject.h>

#include <vector>
#include <fstream>

class CsvWriter : public JEventProcessor {
private:

    // Define the parameters that our processor is going to request

    Parameter<std::vector<std::string>> m_collection_names {this, "csv:collection_names", {}, 
        "Comma-separated list of collection names (Type:Tag) to write to CSV"};

    Parameter<std::string> m_dest_path{this, "csv:dest_file", "./output.csv", 
        "Location where CSV files get written"};

    // Other internal member variables that our EventProcessor needs

    std::vector<std::pair<std::string, std::string>> m_collection_types_and_tags;

    std::fstream m_dest_file;

public:

    CsvWriter() {
        SetTypeName(NAME_OF_THIS);
        SetCallbackStyle(CallbackStyle::ExpertMode);
    };

    void Init() override {

        m_dest_file.open(*m_dest_path, std::fstream::out);

        // For historical reasons, our JEvent interface requires types and tags to be separated
        // for non-Podio types. Ideally we could deprecate this in favor of a unified concept of
        // collection name that includes both type and tag information. However this takes some finesse.
        // So for now, we do the separation here in order to minimize runtime costs.
        for (auto collection_name : *m_collection_names) {
            auto pos = collection_name.find(":");
            std::string type_name = collection_name.substr(0, pos);
            std::string tag = (pos==std::string::npos ? "":collection_name.substr(pos+1));
            // Note that we do this exact same thing in JControlEventProcessor.cc:158. This should be refactored in both
            // places once the JStorage mechanism is fully implemented and merged.

            m_collection_types_and_tags.push_back({type_name, tag});
            LOG_INFO(GetLogger()) << "Requesting collection with type=" << type_name << ", tag=" << tag << LOG_END;
        }
    }

    void ProcessParallel(const JEvent& event) override {

        // In parallel, trigger construction of the collections we need.
        for (const auto& [type_name, tag] : m_collection_types_and_tags) {
            auto fac = event.GetFactorySet()->GetFactory(type_name, tag);
            if (fac == nullptr) {
                // If the factory is not found, throw an exception immediately and exit
                throw JException("Factory not found! typename=%s, tag=%s", type_name.c_str(), tag.c_str());
            }
            fac->Create(event);
        }
    }

    void ProcessSequential(const JEvent& event) override {

        // Sequentially, read the collections we requested earlier, and write them to file.
        // Everything inside this callback happens inside a lock; unlike earlier versions
        // of JANA, users do not need to manipulate locks at all. To get this behavior,
        // set the callback style to "ExpertMode" in the constructor.

        m_dest_file
            << "########################################" << std::endl
            << "event_number,run_number" << std::endl
            << event.GetEventNumber() << "," << event.GetRunNumber() << std::endl;

        for (const auto& [type_name, tag] : m_collection_types_and_tags) {

            // Retrieve the collections we triggered earlier
            auto fac = event.GetFactorySet()->GetFactory(type_name, tag);
            std::vector<JObject*> objects = fac->GetAs<JObject>();
            size_t obj_count = objects.size();
            if (obj_count != fac->GetNumObjects()) {
                throw JException("Collection does not appear to contain JObjects!");
                // Right now, there's no foolproof way to distinguish between 
                // JFactory::GetAs() returning empty because the collection is simply empty,
                // or because the objects don't inherit from JObject. This is another thing
                // to address once JStorage is in place.
            }

            // Print collection information
            m_dest_file
                << "========================================" << std::endl
                << "type_name,tag,object_count" << std::endl
                << type_name << "," << tag << "," << obj_count << std::endl
                << "----------------------------------------" << std::endl;

            JObjectSummary summary;

            // Print header
            if (obj_count > 0) {
                objects[0]->Summarize(summary);
                for (auto& field : summary.get_fields()) {
                    m_dest_file << field.name << ",";
                }
                m_dest_file << std::endl;
            }

            // Print each row in the table
            for (auto obj : objects) {
                obj->Summarize(summary);
                for (auto& field : summary.get_fields()) {
                    m_dest_file << field.value << ",";
                }
                m_dest_file << std::endl;
            }
        }
    }

    void Finish() override {
        //m_dest_file.close();
    }

};

extern "C" {
void InitPlugin(JApplication* app) {
    InitJANAPlugin(app);
    app->Add(new CsvWriter);
}
}




