// Copyright 2023, Jefferson Science Associates, LLC.
// Subject to the terms in the LICENSE file found in the top-level directory.
// Created by Nathan Brei

#pragma once

#include <JANA/JFactorySet.h>
#include <JANA/JFactoryGenerator.h>
#include <vector>

namespace jana::components {

template<class FactoryT>
class JOmniFactoryGeneratorT : public JFactoryGenerator {
public:
    using FactoryConfigType = typename FactoryT::ConfigType;

    struct TypedWiring {
        std::string tag = "";
        JEventLevel level = JEventLevel::PhysicsEvent;
        std::vector<std::string> input_names = {};
        std::vector<JEventLevel> input_levels = {};
        std::vector<std::vector<std::string>> variadic_input_names = {};
        std::vector<JEventLevel> variadic_input_levels = {};
        std::vector<std::string> output_names = {};
        FactoryConfigType configs = {}; /// Must be copyable!
    };

public:

    explicit JOmniFactoryGeneratorT() = default;

    explicit JOmniFactoryGeneratorT(std::string tag,
                                    std::vector<std::string> input_names,
                                    std::vector<std::string> output_names,
                                    FactoryConfigType configs) {
        m_typed_wirings.push_back({.tag=tag,
                                   .input_names=input_names,
                                   .output_names=output_names,
                                   .configs=configs
                                  });
    };

    explicit JOmniFactoryGeneratorT(std::string tag,
                                    std::vector<std::string> input_names,
                                    std::vector<std::string> output_names) {
        m_typed_wirings.push_back({.tag=tag,
                                   .input_names=input_names,
                                   .output_names=output_names
                                  });

    }

    explicit JOmniFactoryGeneratorT(TypedWiring&& wiring) {
        m_typed_wirings.push_back(std::move(wiring));
    }


    void AddWiring(std::string tag,
                   std::vector<std::string> input_names,
                   std::vector<std::string> output_names,
                   FactoryConfigType configs={}) {

        m_typed_wirings.push_back({.tag=tag,
                                   .input_names=input_names,
                                   .output_names=output_names,
                                   .configs=configs
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
            factory->SetTypeName(JTypeInfo::demangle<FactoryT>());
            factory->config() = wiring.configs;

            // Set up all of the wiring prereqs so that Init() can do its thing
            // Specifically, it needs valid input/output tags, a valid logger, and
            // valid default values in its Config object
            factory->PreInit(wiring.tag, wiring.level, wiring.input_names, wiring.input_levels, 
                             wiring.variadic_input_names, wiring.variadic_input_levels, wiring.output_names);

            // Factory is ready
            factory_set->Add(factory);
        }
    }

private:
    std::vector<TypedWiring> m_typed_wirings;
};

} // namespace jana::components
using jana::components::JOmniFactoryGeneratorT;
