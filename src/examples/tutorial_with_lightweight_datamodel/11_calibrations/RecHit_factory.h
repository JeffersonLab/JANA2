
#pragma once
#include <JANA/JFactory.h>
#include "TranslationTable_service.h"
#include <CalorimeterHit.h>
#include <ADCPulse.h>


class RecHit_factory : public JFactory {

private:

    VariadicInput<ADCPulse> m_adc_pulses_in {this};


    VariadicOutput<CalorimeterHit> m_calo_hits_out {this};


    Service<TranslationTable_service> m_translation_table{this};

public:

    RecHit_factory();

    void Process(const JEvent& event) override;

};



