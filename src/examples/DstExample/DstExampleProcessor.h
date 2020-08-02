
// Copyright 2020, Jefferson Science Associates, LLC.
// Subject to the terms in the LICENSE file found in the top-level directory.


#ifndef _DstExampleProcessor_h_
#define _DstExampleProcessor_h_

#include <JANA/JEventProcessor.h>

class DstExampleProcessor : public JEventProcessor {

    // Shared state (e.g. histograms, TTrees, TFiles) live
    std::mutex m_mutex;
    
public:

    DstExampleProcessor();
    virtual ~DstExampleProcessor() = default;

    void Init() override;
    void Process(const std::shared_ptr<const JEvent>& event) override;
    void Finish() override;

};


#endif // _DstExampleProcessor_h_

