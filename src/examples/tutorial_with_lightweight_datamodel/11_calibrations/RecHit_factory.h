
#pragma once
#include <JANA/JFactory.h>
#include "TranslationTable_service.h"
#include <CalorimeterHit.h>
#include <ADCPulse.h>


class RecHit_factory : public JFactory {

private:

    VariadicInput<ADCPulse> m_adc_pulses_in {this};

    VariadicOutput<CalorimeterHit> m_calo_hits_out {this};

    Service<TranslationTable_service> m_translation_table_svc {this};

    TranslationTable_service::DAQLookupTable m_lookup_table;

public:

    RecHit_factory();

    void ChangeRun(const JEvent& event) override;

    void Process(const JEvent& event) override;

};



