
// Copyright 2023, Jefferson Science Associates, LLC.
// Subject to the terms in the LICENSE file found in the top-level directory.

#include <JANA/JFactory.h>
#include <JANA/JEvent.h>

#ifdef HAVE_PODIO
#include <podio/Frame.h>
// So that JTypeInfo::demangle() can RTTI podio::Frame
#endif


void JFactory::Create(const std::shared_ptr<const JEvent>& event) {

    // Make sure that we have a valid JApplication before attempting to call callbacks
    if (mApp == nullptr) mApp = event->GetJApplication();
    auto run_number = event->GetRunNumber();

    if (mStatus == Status::Uninitialized) {
        try {
            std::call_once(mInitFlag, &JFactory::Init, this);
            mStatus = Status::Unprocessed;
        }
        catch (JException& ex) {
            ex.plugin_name = mPluginName;
            ex.component_name = mFactoryName;
            throw ex;
        }
        catch (...) {
            auto ex = JException("Unknown exception in JFactoryT::Init()");
            ex.nested_exception = std::current_exception();
            ex.plugin_name = mPluginName;
            ex.component_name = mFactoryName;
            throw ex;
        }
    }
    if (mStatus == Status::Unprocessed) {
        if (mPreviousRunNumber == -1) {
            // This is the very first run
            ChangeRun(event);
            BeginRun(event);
            mPreviousRunNumber = run_number;
        }
        else if (mPreviousRunNumber != run_number) {
            // This is a later run, and it has changed
            EndRun();
            ChangeRun(event);
            BeginRun(event);
            mPreviousRunNumber = run_number;
        }
#ifdef HAVE_PODIO
        if (mFrameNeeded) {
            // Retrieve the frame from the event object. If it doesn't exist, create and insert it
            try {
                mFrame = const_cast<podio::Frame*>(event->GetSingle<podio::Frame>(""));
            }
            catch (...) {
                mFrame = new podio::Frame;
                event->Insert(mFrame);
            }
        }
#endif
        Process(event);
        mStatus = Status::Processed;
        mCreationStatus = CreationStatus::Created;
    }
}
