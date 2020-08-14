
// Copyright 2020, Jefferson Science Associates, LLC.
// Subject to the terms in the LICENSE file found in the top-level directory.

#ifndef _TrackSmearingFactory_h_
#define _TrackSmearingFactory_h_

#include <JANA/JFactoryT.h>

#include "Track.h"
#include "TrackMetadata.h"

class TrackSmearingFactory : public JFactoryT<Track> {

    // Insert any member variables here

public:
    TrackSmearingFactory() {
        SetTag("smeared");
    };

    void Init() override;
    void ChangeRun(const std::shared_ptr<const JEvent> &event) override;
    void Process(const std::shared_ptr<const JEvent> &event) override;

};

#endif // _TrackSmearingFactory_h_
