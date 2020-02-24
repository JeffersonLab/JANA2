

#ifndef _RandomSource_h_
#define  _RandomSource_h_

#include <JANA/JEventSource.h>
#include <JANA/JEventSourceGeneratorT.h>

class RandomSource : public JEventSource {

    int m_max_emit_freq_hz = 100;

public:
    RandomSource(std::string resource_name, JApplication* app);

    virtual ~RandomSource() = default;

    void Open() override;

    void GetEvent(std::shared_ptr<JEvent>) override;
    
    static std::string GetDescription();

};

template <>
double JEventSourceGeneratorT<RandomSource>::CheckOpenable(std::string);

#endif // _RandomSource_h_

