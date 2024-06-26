// Copyright 2020, Jefferson Science Associates, LLC.
// Subject to the terms in the LICENSE file found in the top-level directory.

#ifndef _JTestRootEventSource_h_
#define  _JTestRootEventSource_h_

#include <random>

#include <JANA/JEventSource.h>
#include <JANA/JEventSourceGeneratorT.h>

class JTestRootEventSource : public JEventSource {

public:
    JTestRootEventSource();
    virtual ~JTestRootEventSource() = default;

    Result Emit(JEvent& event) override;

protected:
    std::default_random_engine generator;
};

#endif // _JTestRootEventSource_h_

