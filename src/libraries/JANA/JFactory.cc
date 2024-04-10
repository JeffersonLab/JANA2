
// Copyright 2023, Jefferson Science Associates, LLC.
// Subject to the terms in the LICENSE file found in the top-level directory.

#include <JANA/JFactory.h>
#include <JANA/JEvent.h>


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
            if (ex.plugin_name.empty()) ex.plugin_name = mPluginName;
            if (ex.component_name.empty()) ex.component_name = mFactoryName;
            if (ex.factory_name.empty()) ex.factory_name = mFactoryName;
            if (ex.factory_tag.empty()) ex.factory_tag = mTag;
            throw ex;
        }
        catch (...) {
            auto ex = JException("Unknown exception in JFactoryT::Init()");
            ex.nested_exception = std::current_exception();
            ex.plugin_name = mPluginName;
            ex.component_name = mFactoryName;
            ex.factory_name = mFactoryName;
            ex.factory_tag = mTag;
            throw ex;
        }
    }

    if (TestFactoryFlag(REGENERATE) && (mStatus == Status::Inserted)) {
        ClearData();
        // ClearData will reset mStatus to Status::Unprocessed
    }

    if (mStatus == Status::Unprocessed) {
        try {
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
        }
        catch (JException& ex) {
            if (ex.plugin_name.empty()) ex.plugin_name = mPluginName;
            if (ex.component_name.empty()) ex.component_name = mFactoryName;
            if (ex.factory_name.empty()) ex.factory_name = mFactoryName;
            if (ex.factory_tag.empty()) ex.factory_tag = mTag;
            throw ex;
        }
        catch (...) {
            auto ex = JException("Unknown exception in JFactoryT::BeginRun/ChangeRun/EndRun()");
            ex.nested_exception = std::current_exception();
            ex.plugin_name = mPluginName;
            ex.component_name = mFactoryName;
            ex.factory_name = mFactoryName;
            ex.factory_tag = mTag;
            throw ex;
        }
        try {
            Process(event);
        }
        catch (JException& ex) {
            if (ex.plugin_name.empty()) ex.plugin_name = mPluginName;
            if (ex.component_name.empty()) ex.component_name = mFactoryName;
            if (ex.factory_name.empty()) ex.factory_name = mFactoryName;
            if (ex.factory_tag.empty()) ex.factory_tag = mTag;
            throw ex;
        }
        catch (...) {
            auto ex = JException("Unknown exception in JFactoryT::Process()");
            ex.nested_exception = std::current_exception();
            ex.plugin_name = mPluginName;
            ex.component_name = mFactoryName;
            ex.factory_name = mFactoryName;
            ex.factory_tag = mTag;
            throw ex;
        }
        mStatus = Status::Processed;
        mCreationStatus = CreationStatus::Created;
    }
}
