
// Copyright 2020, Jefferson Science Associates, LLC.
// Subject to the terms in the LICENSE file found in the top-level directory.


#ifndef _RandomSource_h_
#define  _RandomSource_h_

#include <JANA/JEventSource.h>
#include <JANA/JEventSourceGeneratorT.h>

class RandomSource : public JEventSource {

    int m_max_emit_freq_hz = 100;

public:
    RandomSource();

    RandomSource(std::string resource_name, JApplication* app);

    virtual ~RandomSource() = default;

    void Open() override;

    Result Emit(JEvent&) override;

    void Close() override;
    
    static std::string GetDescription();

};

template <>
double JEventSourceGeneratorT<RandomSource>::CheckOpenable(std::string);

#endif // _RandomSource_h_

