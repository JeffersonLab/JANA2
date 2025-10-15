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

#include <string>
#include <vector>

namespace jana::components {

struct EmptyConfig {};

template <typename AlgoT, typename ConfigT=EmptyConfig>
class JOmniFactory : public JMultifactory {
private:

    ConfigT m_config;

public:

    void Init() override {
        static_cast<AlgoT*>(this)->Configure();
    }

    void ChangeRun(const std::shared_ptr<const JEvent>& event) override {
        static_cast<AlgoT*>(this)->ChangeRun(event->GetRunNumber());
    }

    void ChangeRun(int32_t) {};

    void Process(const std::shared_ptr<const JEvent> &event) override {
        static_cast<AlgoT*>(this)->Execute(event->GetRunNumber(), event->GetEventNumber());
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


