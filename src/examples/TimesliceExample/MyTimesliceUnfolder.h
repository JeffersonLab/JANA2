// Copyright 2024, Jefferson Science Associates, LLC.
// Subject to the terms in the LICENSE file found in the top-level directory.

#pragma once
#include "MyDataModel.h"
#include <JANA/JEventUnfolder.h>

struct ExampleTimesliceUnfolder : public JEventUnfolder {

    MyTimesliceUnfolder() {
        SetParentLevel(JEventLevel::Timeslice);
        SetChildLevel(JEventLevel::Event);
    }
    
    void Preprocess(const JEvent& parent) const override {
        parent->Get<MyCluster>("protos");
    }

    Result Unfold(const JEvent& parent, JEvent& child, int item) override {
        auto protos = parent->Get<MyCluster>("protos");

        child.SetEventNumber(parent.GetEventNumber()*10 + item);
        LOG << "Unfolding parent=" << parent.GetEventNumber() << ", child=" << child.GetEventNumber() << ", item=" << item << LOG_END;

        std::vector<MyCluster*> child_protos;
        for (auto proto: protos) {
            if (true) {
                // TODO: condition
                child_protos.push_back(proto);
            }
        }
        child->Insert(child_protos, "event_protos")->SetFactoryFlag(JFactoryFlag::NOT_OBJECT_OWNER);

        if (item == 3) {
            jout << "Unfold found item 3, finishing join" << jendl;
            return Result::Finished;
        }
        return Result::KeepGoing;
    }
}



