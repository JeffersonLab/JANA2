// Copyright 2020, Jefferson Science Associates, LLC.
// Subject to the terms in the LICENSE file found in the top-level directory.

#ifndef _JTestRootProcessor_h_
#define _JTestRootProcessor_h_

#include <JANA/JEventProcessor.h>

class JTestRootProcessor : public JEventProcessor {

    // Shared state (e.g. histograms, TTrees, TFiles) live
    
public:

    JTestRootProcessor();
    virtual ~JTestRootProcessor() = default;

    void Process(const JEvent&) override;
};


#endif // _JTestRootProcessor_h_

