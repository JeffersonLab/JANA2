// Copyright 2025, Jefferson Science Associates, LLC.
// Subject to the terms in the LICENSE file found in the top-level directory.
// Created by Nathan Brei

#pragma once

#include <JANA/JFactorySet.h>
#include <JANA/JFactoryGenerator.h>
#include <JANA/Services/JWiringService.h>


namespace jana::components {

template<class FactoryT>
class JWiredFactoryGeneratorT : public JFactoryGenerator {

public:
    using FactoryConfigType = typename FactoryT::ConfigType;

    explicit JWiredFactoryGeneratorT() = default;

    void GenerateFactories(JFactorySet *factory_set) override {

        auto wiring_svc = GetApplication()->template GetService<jana::services::JWiringService>();
        const auto& shared_params = wiring_svc->GetSharedParameters();

        const auto& type_name = JTypeInfo::demangle<FactoryT>();
        for (const auto* wiring : wiring_svc->GetWirings(GetPluginName(), type_name)) {

            FactoryT *factory = new FactoryT;
            factory->SetApplication(GetApplication());
            factory->SetPluginName(this->GetPluginName());
            factory->SetTypeName(type_name);

            // Set the parameter values on the factory. This way, the values in the wiring file
            // show up as "defaults" and the values set on the command line show up as "overrides".
            for (auto parameter : factory->GetAllParameters()) {
                parameter->Wire(wiring->configs, shared_params);
            }

            // Check that output levels in wiring file match the factory's level
            for (auto output_level : wiring->output_levels) {
                if (output_level != wiring->level) {
                    throw JException("JOmniFactories are constrained to a single output level");
                }
            }

            factory->PreInit(wiring->prefix,
                             wiring->level,
                             wiring->input_names,
                             wiring->input_levels,
                             wiring->variadic_input_names,
                             wiring->variadic_input_levels,
                             wiring->output_names);

            factory_set->Add(factory);
        }
    }
};

} // namespace jana::components
using jana::components::JWiredFactoryGeneratorT;
