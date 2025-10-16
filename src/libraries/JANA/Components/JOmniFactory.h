// Copyright 2023, Jefferson Science Associates, LLC.
// Subject to the terms in the LICENSE file found in the top-level directory.
// Created by Nathan Brei

#pragma once

/**
 * Omnifactories are a lightweight layer connecting JANA to generic algorithms
 * It is assumed multiple input data (controlled by input tags)
 * which might be changed by user parameters.
 */

#include <JANA/JEvent.h>
#include <JANA/JFactory.h>

namespace jana::components {

struct EmptyConfig {};

template <typename FacT, typename ConfigT=EmptyConfig>
class JOmniFactory : public JFactory {
private:

    ConfigT m_config;

public:

    JOmniFactory() {
        SetCallbackStyle(CallbackStyle::LegacyMode);
    }

    void Init() override {
        static_cast<FacT*>(this)->Configure();
    }

    void BeginRun(const std::shared_ptr<const JEvent>& event) override {
        // UserFac::ChangeRun() called from BeginRun() to avoid overloaded-virtual warning
        static_cast<FacT*>(this)->ChangeRun(event->GetRunNumber());
    }

    void Process(const std::shared_ptr<const JEvent>& event) override {
        static_cast<FacT*>(this)->Execute(event->GetRunNumber(), event->GetEventNumber());
    }

    // This is more hackery to suppress the overloaded-virtual warning
    // Has to virtual simply because EICrecon already declares it as override
    using JFactory::ChangeRun;
    void ChangeRun(int32_t) {}; 


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


