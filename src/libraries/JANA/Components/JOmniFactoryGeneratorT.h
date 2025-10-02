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
        std::vector<std::vector<std::string>> variadic_output_names = {};
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

        if (m_typed_wirings.size() == 0) {
            FactoryT *factory = new FactoryT;
            factory->SetPluginName(this->GetPluginName());
            factory->SetTypeName(JTypeInfo::demangle<FactoryT>());
            factory->Wire(GetApplication());
            factory_set->Add(factory);
        }

        for (auto& wiring : m_typed_wirings) {

            auto prefix = (GetPluginName().empty()) ? wiring.tag : this->GetPluginName() + ":" + wiring.tag;
            GetApplication()->SetDefaultParameter(prefix + ":InputTags", wiring.input_names, "Input collection names");
            GetApplication()->SetDefaultParameter(prefix + ":OutputTags", wiring.output_names, "Output collection names");

            FactoryT *factory = new FactoryT;
            factory->SetPluginName(this->GetPluginName());
            factory->SetTypeName(JTypeInfo::demangle<FactoryT>());
            factory->SetPrefix(prefix);

            // Apply the wiring information passed to the generator
            factory->SetLevel(wiring.level);
            factory->WireInputs(wiring.level, wiring.input_levels, wiring.input_names, wiring.variadic_input_levels, wiring.variadic_input_names);
            factory->WireOutputs(wiring.level, wiring.output_names, wiring.variadic_output_names, false);
            factory->config() = wiring.configs;

            // Apply the wiring information from file
            factory->Wire(GetApplication());

            // Factory is ready
            factory_set->Add(factory);
        }
    }

private:
    std::vector<TypedWiring> m_typed_wirings;
};

} // namespace jana::components
using jana::components::JOmniFactoryGeneratorT;
