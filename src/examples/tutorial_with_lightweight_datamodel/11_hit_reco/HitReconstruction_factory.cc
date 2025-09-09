
#include "HitReconstruction_factory.h"
#include "ADCPulse.h"
#include <JANA/JEvent.h>
#include "CalorimeterHit.h"

HitReconstruction_factory::HitReconstruction_factory() {
    m_adc_pulses_in.SetRequestedDatabundleNames({"raw"});
    m_calo_hits_out.SetShortNames({"rechits"});
}

void HitReconstruction_factory::ChangeRun(const JEvent& event) {

    // We use ChangeRun to obtain any run-level data we need. This is ONLY called when the run number has changed.
    // We cache the run-encoded data on the factory directly (Remember that there are many factories in memory at any given time!)
    // If the data is large, we use shared_ptrs (under the hood here) to ensure that there is only one copy in memory, and that 
    // it gets deleted once the run number changes.

    m_lookup_table = m_translation_table_svc->GetDAQLookupTable(event.GetRunNumber());
}

void HitReconstruction_factory::Process(const JEvent&) {

    // We iterate over EACH input databundle we've been provided
    for (size_t pulse_databundle_index = 0; pulse_databundle_index<m_adc_pulses_in->size(); ++pulse_databundle_index) {


        // Unlike ouptuts and regular inputs, variadic inputs distinguish between 'requested' and 'realized' databundles.
        // This is because variadic inputs may be flexible, i.e. they may be optional, or may use EmptyInputPolicy::IncludeEverything.

        auto name = m_adc_pulses_in.GetRealizedDatabundleNames().at(pulse_databundle_index);

        // In this simple example, we assume a one-to-one correspondence between input ADCPulses and output CalorimeterHits.
        if (m_adc_pulses_in->size() != m_calo_hits_out->size()) {
            throw JException("Found wrong number of ADCPulse inputs!");
        }

        LOG_DEBUG(GetLogger()) << "Reconstructing hits. "
                               << m_adc_pulses_in.GetRealizedDatabundleNames().at(pulse_databundle_index)
                               << " -> "
                               << m_calo_hits_out.GetUniqueNames().at(pulse_databundle_index);

        // Process each pulse in this databundle

        for (const auto* pulse: m_adc_pulses_in->at(pulse_databundle_index)) {

            // Translate from DAQ coordinates to detector coordinates

            auto& row = m_lookup_table->at({pulse->crate, pulse->slot, pulse->channel});
            auto& detector_coords = std::get<0>(row);
            auto& calib = std::get<1>(row);

            auto hit = new CalorimeterHit(detector_coords.cell_id, 
                                                            detector_coords.indices.at(0), detector_coords.indices.at(1), 
                                                            detector_coords.x, detector_coords.y, detector_coords.z, 
                                                            0, 0);

            // Apply gains and offsets

            hit->energy = ((pulse->integral - pulse->pedestal) * calib.gain) - calib.pedestal;
            hit->time = (pulse->timestamp * calib.tick_period) - calib.time_offset;

            // Add hit to corresponding output databundle

            m_calo_hits_out->at(pulse_databundle_index).push_back(hit);
        }
    }
}

