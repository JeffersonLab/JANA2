// Copyright 2020, Jefferson Science Associates, LLC.
// Subject to the terms in the LICENSE file found in the top-level directory.

#ifndef _JTestRootEventSource_h_
#define  _JTestRootEventSource_h_

#include <random>

#include <JANA/JEventSource.h>
#include <JANA/JEventSourceGeneratorT.h>
#include <JANA/Utils/JBenchUtils.h>


class JTestRootEventSource : public JEventSource {

    JBenchUtils m_bench_utils = JBenchUtils();
public:
    JTestRootEventSource();
    virtual ~JTestRootEventSource() = default;

    Result Emit(JEvent& event) override;

protected:
    std::default_random_engine generator;
};

#endif // _JTestRootEventSource_h_

