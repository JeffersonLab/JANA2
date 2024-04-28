
// Copyright 2020, Jefferson Science Associates, LLC.
// Subject to the terms in the LICENSE file found in the top-level directory.

#ifndef JANA2_USEREXCEPTIONTESTS_H
#define JANA2_USEREXCEPTIONTESTS_H


#include <JANA/JEventSource.h>
#include <JANA/JEventProcessor.h>



struct FlakySource : public JEventSource {

    bool open_excepts, getevent_excepts;
    int event_count = 0;

    FlakySource(bool open_excepts, bool getevent_excepts)
            : open_excepts(open_excepts), getevent_excepts(getevent_excepts) {
                SetCallbackStyle(CallbackStyle::ExpertMode);
            }

    void Open() override {
        if (open_excepts) {
            throw JException("Unable to open source!");
        }
    }

    Result Emit(JEvent&) override {

        if (++event_count > 10) {
            return Result::FailureFinished;
        }

        if (getevent_excepts) {
            throw JException("Unable to getEvent!");
        }
        return Result::Success;
    }
};



struct FlakyProcessor : public JEventProcessor {

    bool init_excepts, process_excepts, finish_excepts;

    FlakyProcessor(bool init_excepts, bool process_excepts, bool finish_excepts)
        : init_excepts(init_excepts)
        , process_excepts(process_excepts)
        , finish_excepts(finish_excepts)
        {
            SetCallbackStyle(CallbackStyle::ExpertMode);
        };

    void Init() override {
        if (init_excepts) {
            throw JException("Unable to init!");
        }
    };

    void Process(const JEvent&) override {
        if (process_excepts) {
            throw JException("Unable to process!");
        }
    }

    void Finish() override {
        if (finish_excepts) {
            throw JException("Unable to finish!");
        }
    }
};


#endif //JANA2_USEREXCEPTIONTESTS_H
