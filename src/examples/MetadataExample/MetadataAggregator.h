
#ifndef _MetadataExampleProcessor_h_
#define _MetadataExampleProcessor_h_

#include <JANA/JEventProcessor.h>

class MetadataAggregator : public JEventProcessor {

    // Shared state (e.g. histograms, TTrees, TFiles) live
    std::mutex m_mutex;
    
public:

    MetadataAggregator();
    virtual ~MetadataAggregator() = default;

    void Init() override;
    void Process(const std::shared_ptr<const JEvent>& event) override;
    void Finish() override;

};


#endif // _MetadataExampleProcessor_h_

