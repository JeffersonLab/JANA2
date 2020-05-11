

#ifndef _RandomTrackSource_h_
#define  _RandomTrackSource_h_

#include <JANA/JEventSource.h>
#include <JANA/JEventSourceGeneratorT.h>

class RandomTrackSource : public JEventSource {

    /// Add member variables here

public:
    RandomTrackSource(std::string resource_name, JApplication* app);

    virtual ~RandomTrackSource() = default;

    void Open() override;

    void GetEvent(std::shared_ptr<JEvent>) override;
    
    static std::string GetDescription();

};

template <>
double JEventSourceGeneratorT<RandomTrackSource>::CheckOpenable(std::string);

#endif // _RandomTrackSource_h_

