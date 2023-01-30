
// Copyright 2023, Jefferson Science Associates, LLC.
// Subject to the terms in the LICENSE file found in the top-level directory.


#ifndef JANA2_JEVENTSOURCEPODIO_H
#define JANA2_JEVENTSOURCEPODIO_H

#include <JANA/JEventSource.h>
#include "PodioFrame.h"


class JEventSourcePodio : public JEventSource {

    JEventSourcePodio(std::string filename);

    /// User should NOT override GetEvent, because the way that we store
    /// the PODIO frame is part of the contract between JANA and PODIO. We
    /// don't prevent users from accessing the PODIO frame directly, but
    /// strongly discourage them from using this for anything other than debugging.
    void GetEvent(std::shared_ptr<JEvent>) final;


    /// User overrides NextFrame so that they can populate the frame however
    /// they like. This lets us support features such as:
    /// - datamodel glue (generated podio helper methods)
    /// - background events
    /// Event index is like event number except it starts at zero and increments.
    /// It is equivalent to Podio's record index.
    virtual std::unique_ptr<PodioFrame> NextFrame(int event_index) = 0;
    // TODO: How do we get event and run numbers out of the ROOT file?
    //       Do these have a slot in the podio frame? I hope they do.
    //       We may need to add them, though.

    /// User may override Open() in case they need multiple files open concurrently,
    /// e.g. for background events. The existing implementation assumes exactly
    /// one file, in ROOT format.
    void Open() override;

    /// User may override Close() in case they need multiple files open concurrently,
    /// e.g. for background events. The existing implementation assumes exactly
    /// one file, in ROOT format.
    void Close() override;

};


#endif //JANA2_JEVENTSOURCEPODIO_H
