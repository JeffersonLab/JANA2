
// Copyright 2020, Jefferson Science Associates, LLC.
// Subject to the terms in the LICENSE file found in the top-level directory.

#ifndef _TutorialProcessor_h_
#define _TutorialProcessor_h_

#include <JANA/JEventProcessor.h>

class TutorialProcessor : public JEventProcessor {

    /// Shared state (e.g. histograms, TTrees, TFiles) live here
    double m_heatmap[100][100];
    
public:

    TutorialProcessor();
    virtual ~TutorialProcessor() = default;

    void Init() override;
    void Process(const std::shared_ptr<const JEvent>& event) override;
    void Finish() override;

};


#endif // _TutorialProcessor_h_

