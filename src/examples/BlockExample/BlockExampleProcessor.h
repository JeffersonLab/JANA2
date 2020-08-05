
#ifndef _BlockExampleProcessor_h_
#define _BlockExampleProcessor_h_

#include <JANA/JEventProcessor.h>

class BlockExampleProcessor : public JEventProcessor {

    // Shared state (e.g. histograms, TTrees, TFiles) live
    std::mutex m_mutex;
    
public:

    BlockExampleProcessor();
    virtual ~BlockExampleProcessor() = default;

    void Init() override;
    void Process(const std::shared_ptr<const JEvent>& event) override;
    void Finish() override;

};


#endif // _BlockExampleProcessor_h_

