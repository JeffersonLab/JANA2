
// Copyright 2020, Jefferson Science Associates, LLC.
// Subject to the terms in the LICENSE file found in the top-level directory.



#ifndef _DstExampleSource_h_
#define  _DstExampleSource_h_

#include <JANA/JEventSource.h>
#include <JANA/JEventSourceGeneratorT.h>

class DstExampleSource : public JEventSource {

    /// Add member variables here

public:
    DstExampleSource(std::string resource_name, JApplication* app);

    virtual ~DstExampleSource() = default;

    void Open() override;

    void GetEvent(std::shared_ptr<JEvent>) override;
    
    static std::string GetDescription();

};

template <>
double JEventSourceGeneratorT<DstExampleSource>::CheckOpenable(std::string);

#endif // _DstExampleSource_h_

