

#ifndef _RandomTrackSource_h_
#define  _RandomTrackSource_h_

#include <JANA/JEventSource.h>
#include <JANA/JEventSourceGeneratorT.h>

class RandomTrackSource : public JEventSource {

    int m_events_in_run;
    int m_current_run_number;
    int m_current_event_number;
    int m_max_run_number;

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

