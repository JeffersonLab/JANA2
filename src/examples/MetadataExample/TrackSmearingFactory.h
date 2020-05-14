
#ifndef _TrackSmearingFactory_h_
#define _TrackSmearingFactory_h_

#include <JANA/JFactoryT.h>

#include "Track.h"
#include "TrackMetadata.h"

class TrackSmearingFactory : public JFactoryT<Track> {

    // Insert any member variables here

public:
    TrackSmearingFactory() : JFactoryT<Track>(NAME_OF_THIS, "smeared") {};
    void Init() override;
    void ChangeRun(const std::shared_ptr<const JEvent> &event) override;
    void Process(const std::shared_ptr<const JEvent> &event) override;

};

#endif // _TrackSmearingFactory_h_
