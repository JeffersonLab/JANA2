
// Copyright 2020, Jefferson Science Associates, LLC.
// Subject to the terms in the LICENSE file found in the top-level directory.

#include "DstExampleFactory.h"
#include "DataObjects.h"

#include <JANA/JEvent.h>

void DstExampleFactory::Init() {

    /// This is necessary to generate the virtual function which does the conversion.
    /// Otherwise your datatype won't show up when you call event->GetAllChildren<T>.
    /// You can call this from the ctor, from Init, or from JEventSource::GetEvent.

    EnableGetAs<JObject>();
    EnableGetAs<Renderable>();
}

void DstExampleFactory::ChangeRun(const std::shared_ptr<const JEvent> &event) {
}

void DstExampleFactory::Process(const std::shared_ptr<const JEvent> &event) {

    auto inputs = event->Get<MyRenderableJObject>("from_source");

    std::vector<MyRenderableJObject*> results;

    for (auto input : inputs) {
        // "Smear" each input
        auto mrj = new MyRenderableJObject(input->x + 1, input->y + 1, input->E + 1);
        results.push_back(mrj);
    }

    Set(results);
}
