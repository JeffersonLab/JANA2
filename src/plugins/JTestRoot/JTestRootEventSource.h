// Copyright 2020, Jefferson Science Associates, LLC.
// Subject to the terms in the LICENSE file found in the top-level directory.

#ifndef _JTestRootEventSource_h_
#define  _JTestRootEventSource_h_

#include <random>

#include <JANA/JEventSource.h>
#include <JANA/JEventSourceGeneratorT.h>

class JTestRootEventSource : public JEventSource {

public:
    JTestRootEventSource(std::string resource_name, JApplication* app);
    virtual ~JTestRootEventSource() = default;

    void GetEvent(std::shared_ptr<JEvent>) override;
};

//template <>
//double JEventSourceGeneratorT<JTestRootEventSource>::CheckOpenable(std::string);

#endif // _JTestRootEventSource_h_

