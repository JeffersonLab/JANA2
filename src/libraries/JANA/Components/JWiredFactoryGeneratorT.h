// Copyright 2025, Jefferson Science Associates, LLC.
// Subject to the terms in the LICENSE file found in the top-level directory.
// Created by Nathan Brei

#pragma once

#include <JANA/Services/JParameterManager.h>
#include <JANA/JFactorySet.h>
#include <JANA/JFactoryGenerator.h>
#include <JANA/Services/JWiringService.h>


namespace jana::components {

template<class FactoryT>
class JWiredFactoryGeneratorT : public JFactoryGenerator {

public:
    explicit JWiredFactoryGeneratorT() = default;

    void GenerateFactories(JFactorySet *factory_set) override {

        auto wiring_svc = GetApplication()->template GetService<jana::services::JWiringService>();
        const auto& type_name = JTypeInfo::demangle<FactoryT>();

        for (const auto& prefix : wiring_svc->GetPrefixesForAddedInstances(GetPluginName(), type_name)) {

            FactoryT *factory = new FactoryT;
            factory->SetPluginName(GetPluginName());
            factory->SetTypeName(type_name);
            factory->SetPrefix(prefix);
            factory->Wire(GetApplication());
            factory_set->Add(factory);
        }
    }
};

} // namespace jana::components
using jana::components::JWiredFactoryGeneratorT;
