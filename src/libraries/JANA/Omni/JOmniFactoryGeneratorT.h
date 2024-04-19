// Copyright 2023, Jefferson Science Associates, LLC.
// Subject to the terms in the LICENSE file found in the top-level directory.
// Created by Nathan Brei

#pragma once

#include <JANA/JFactorySet.h>
#include <JANA/JFactoryGenerator.h>
#include <vector>

template<class FactoryT>
class JOmniFactoryGeneratorT : public JFactoryGenerator {
public:
    using FactoryConfigType = typename FactoryT::ConfigType;

    struct TypedWiring {
        std::string tag = "";
        JEventLevel level = JEventLevel::Event;
        std::vector<std::string> input_tags = {};
        std::vector<JEventLevel> input_levels = {};
        std::vector<std::string> output_tags = {};
        FactoryConfigType configs = {}; /// Must be copyable!
    };

    struct UntypedWiring {
        std::string tag = "";
        JEventLevel level = JEventLevel::Event;
        std::vector<std::string> input_tags = {};
        std::vector<JEventLevel> input_levels = {};
        std::vector<std::string> output_tags = {};
        std::map<std::string, std::string> configs = {};
    };

public:

    explicit JOmniFactoryGeneratorT() = default;

    explicit JOmniFactoryGeneratorT(std::string tag,
                                    std::vector<std::string> default_input_tags,
                                    std::vector<std::string> default_output_tags,
                                    FactoryConfigType configs) {
        m_typed_wirings.push_back({.tag=tag,
                             .input_tags=default_input_tags,
                             .output_tags=default_output_tags,
                             .configs=configs
                            });
    };

    explicit JOmniFactoryGeneratorT(std::string tag,
                                    std::vector<std::string> default_input_tags,
                                    std::vector<std::string> default_output_tags) {
        m_typed_wirings.push_back({.tag=tag,
                             .input_tags=default_input_tags,
                             .output_tags=default_output_tags,
                             .configs={}
                            });

    }

    explicit JOmniFactoryGeneratorT(TypedWiring&& wiring) {
        m_typed_wirings.push_back(std::move(wiring));
    }


    void AddWiring(std::string tag,
                   std::vector<std::string> default_input_tags,
                   std::vector<std::string> default_output_tags,
                   FactoryConfigType configs) {

        m_typed_wirings.push_back({.m_tag=tag,
                             .m_default_input_tags=default_input_tags,
                             .m_default_output_tags=default_output_tags,
                             .configs=configs
                            });
    }

    void AddWiring(std::string tag,
                   std::vector<std::string> input_tags,
                   std::vector<std::string> output_tags,
                   std::map<std::string, std::string> configs={}) {

        // Create throwaway factory so we can populate its config using our map<string,string>.
        FactoryT factory;
        factory.ConfigureAllParameters(configs);
        auto configs_typed = factory.config();

        m_typed_wirings.push_back({.tag=tag,
                             .input_tags=input_tags,
                             .output_tags=output_tags,
                             .configs=configs_typed
                            });

    }

    void AddWiring(TypedWiring wiring) {
        m_typed_wirings.push_back(wiring);
    }

    void GenerateFactories(JFactorySet *factory_set) override {

        for (const auto& wiring : m_typed_wirings) {


            FactoryT *factory = new FactoryT;
            factory->SetApplication(GetApplication());
            factory->SetPluginName(this->GetPluginName());
            factory->SetFactoryName(JTypeInfo::demangle<FactoryT>());
            // factory->SetTag(wiring.m_tag);
            // We do NOT want to do this because JMF will use the tag to suffix the collection names
            // TODO: NWB: Change this in JANA
            factory->config() = wiring.configs;

            // Set up all of the wiring prereqs so that Init() can do its thing
            // Specifically, it needs valid input/output tags, a valid logger, and
            // valid default values in its Config object
            factory->PreInit(wiring.tag, wiring.level, wiring.input_tags, wiring.input_levels, wiring.output_tags);

            // Factory is ready
            factory_set->Add(factory);
        }
    }

private:
    std::vector<TypedWiring> m_typed_wirings;
};
