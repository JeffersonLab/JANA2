// Copyright 2023, Jefferson Science Associates, LLC.
// Subject to the terms in the LICENSE file found in the top-level directory.
// Created by Nathan Brei

#pragma once

/**
 * Omnifactories are a lightweight layer connecting JANA to generic algorithms
 * It is assumed multiple input data (controlled by input tags)
 * which might be changed by user parameters.
 */

#include "JANA/Services/JParameterManager.h"
#include "JANA/Utils/JEventLevel.h"
#include <JANA/JEvent.h>
#include <JANA/JMultifactory.h>
#include <JANA/JVersion.h>
#include <JANA/Components/JHasInputs.h>
#include <JANA/Components/JHasOutputs.h>

#include <string>
#include <vector>

namespace jana::components {

struct EmptyConfig {};

template <typename AlgoT, typename ConfigT=EmptyConfig>
class JOmniFactory : public JMultifactory, 
                     public jana::components::JHasInputs,
                     public jana::components::JHasOutputs {
private:

    ConfigT m_config;

public:

    inline void PreInit(std::string tag,
                        JEventLevel level,
                        std::vector<std::string> input_collection_names,
                        std::vector<JEventLevel> input_collection_levels,
                        std::vector<std::vector<std::string>> variadic_input_collection_names,
                        std::vector<JEventLevel> variadic_input_collection_levels, 
                        std::vector<std::string> output_collection_names,
                        std::vector<std::vector<std::string>> variadic_output_collection_names
                        ) {

        m_prefix = (this->GetPluginName().empty()) ? tag : this->GetPluginName() + ":" + tag;
        m_level = level;

        // Obtain logger
        m_logger = m_app->GetService<JParameterManager>()->GetLogger(m_prefix);

        // Obtain collection name overrides if provided.
        // Priority = [JParameterManager, JOmniFactoryGenerator]
        m_app->SetDefaultParameter(m_prefix + ":InputTags", input_collection_names, "Input collection names");
        m_app->SetDefaultParameter(m_prefix + ":OutputTags", output_collection_names, "Output collection names");

        WireInputs(level, input_collection_levels, input_collection_names, variadic_input_collection_levels, variadic_input_collection_names);
        WireOutputs(level, output_collection_names, variadic_output_collection_names);

        // Handle the JMultifactory-specific wiring details
        for (auto* output: m_outputs) {
            output->CreateHelperFactory(*this);
        }

        // Configure logger. Priority = [JParameterManager, system log level]
        // std::string default_log_level = eicrecon::LogLevelToString(m_logger->level());
        // m_app->SetDefaultParameter(m_prefix + ":LogLevel", default_log_level, "LogLevel: trace, debug, info, warn, err, critical, off");
        // m_logger->set_level(eicrecon::ParseLogLevel(default_log_level));
    }

    void Init() override {
        for (auto* parameter : m_parameters) {
            parameter->Init(*(m_app->GetJParameterManager()), m_prefix);
        }
        for (auto* service : m_services) {
            service->Fetch(m_app);
        }
        static_cast<AlgoT*>(this)->Configure();
    }

    void BeginRun(const std::shared_ptr<const JEvent>& event) override {
        for (auto* resource : m_resources) {
            resource->ChangeRun(event->GetRunNumber(), m_app);
        }
        static_cast<AlgoT*>(this)->ChangeRun(event->GetRunNumber());
    }

    void Process(const std::shared_ptr<const JEvent> &event) override {
        try {
            for (auto* input : m_inputs) {
                input->GetCollection(*event);
            }
            for (auto* variadic_input : m_variadic_inputs) {
                variadic_input->GetCollection(*event);
            }
            for (auto* output : m_outputs) {
                output->Reset();
            }
            static_cast<AlgoT*>(this)->Execute(event->GetRunNumber(), event->GetEventNumber());
            for (auto* output : m_outputs) {
                output->SetCollection(*this);
            }
        }
        catch(std::exception &e) {
            throw JException(e.what());
        }
    }

    using ConfigType = ConfigT;

    /// Retrieve reference to already-configured logger
    //std::shared_ptr<spdlog::logger> &logger() { return m_logger; }
    JLogger& logger() { return m_logger; }

    /// Retrieve reference to embedded config object
    ConfigT& config() { return m_config; }


    /// Generate summary for UI, inspector
    void Summarize(JComponentSummary& summary) const override {

        auto* mfs = new JComponentSummary::Component(
            "OmniFactory", GetPrefix(), GetTypeName(), GetLevel(), GetPluginName());

        SummarizeInputs(*mfs);
        SummarizeOutputs(*mfs);
        summary.Add(mfs);
    }

};

} // namespace jana::components

using jana::components::JOmniFactory;


