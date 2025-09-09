
#include "RecHit_factory.h"
#include "ADCPulse.h"
#include <JANA/JEvent.h>
#include "CalorimeterHit.h"

RecHit_factory::RecHit_factory() {
    m_adc_pulses_in.SetRequestedDatabundleNames({"raw"});
    m_calo_hits_out.SetShortNames({"rechits"});
}

void RecHit_factory::Process(const JEvent& event) {

    auto name = m_adc_pulses_in.GetRealizedDatabundleNames();


    // This time we iterate over EACH input databundle we've been provided
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
            auto& detector_coords = m_translation_table->TranslateDAQCoordinates(event.GetRunNumber(), {pulse->crate, pulse->slot, pulse->channel});

            auto hit = new CalorimeterHit(detector_coords.cell_id, 
                                                            detector_coords.indices.at(0), detector_coords.indices.at(1), 
                                                            detector_coords.x, detector_coords.y, detector_coords.z, 
                                                            0, 0);

            // TODO: Apply energy calibrations
            hit->energy = pulse->amplitude;
            hit->time = pulse->timestamp;

            m_calo_hits_out->at(pulse_databundle_index).push_back(hit);
        }
    }
}

