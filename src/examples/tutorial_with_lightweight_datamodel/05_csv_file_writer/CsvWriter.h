
#ifndef _CsvWriter_h_
#define _CsvWriter_h_

#include "EventHeader.h"
#include "CalorimeterHit.h"
#include "CalorimeterCluster.h"
#include "SimParticle.h"
#include "ADCHit.h"

#include <JANA/JEventProcessor.h>

class CsvWriter : public JEventProcessor {

    Parameter<std::string> m_output_filename {this, "output_filename", "output.csv"};

    Input<EventHeader> m_event_header_in {this};
    VariadicInput<CalorimeterHit> m_calo_hit_collections_in {this};
    //VariadicInput<CalorimeterCluster> m_calo_cluster_collections_in {this};
    //VariadicInput<SimParticle> m_sim_particle_collections_in {this};
    //VariadicInput<ADCHit> m_adc_hit_collections_in {this};

    std::ofstream m_output_file;

public:

    CsvWriter();
    virtual ~CsvWriter() = default;

    void Init() override;
    void ProcessSequential(const JEvent& event) override;
    void Finish() override;

};


#endif // _CsvWriter_h_

