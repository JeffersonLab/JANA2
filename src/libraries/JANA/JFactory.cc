
// Copyright 2023, Jefferson Science Associates, LLC.
// Subject to the terms in the LICENSE file found in the top-level directory.

#include <JANA/JFactory.h>
#include <JANA/JEvent.h>
#include <JANA/Utils/JTypeInfo.h>


void JFactory::Create(const std::shared_ptr<const JEvent>& event) {

    // Make sure that we have a valid JApplication before attempting to call callbacks
    if (mApp == nullptr) mApp = event->GetJApplication();
    auto run_number = event->GetRunNumber();

    if (mStatus == Status::Uninitialized) {
        try {
            Init();
            mStatus = Status::Unprocessed;
        }
        catch (JException& ex) {
            std::string instanceName = mTag.empty() ? mObjectName : mObjectName + ":" + mTag;
            if (ex.function_name.empty()) ex.function_name = "JFactory::Init";
            if (ex.type_name.empty()) ex.type_name = mFactoryName;
            if (ex.instance_name.empty()) ex.instance_name = instanceName;
            if (ex.plugin_name.empty()) ex.plugin_name = mPluginName;
            throw ex;
        }
        catch (std::exception& e) {
            auto ex = JException("Exception in JFactoryT::Init(): %s", e.what());
            ex.exception_type = JTypeInfo::demangle_current_exception_type();
            ex.nested_exception = std::current_exception();
            ex.function_name = "JFactory::Init";
            ex.type_name = mFactoryName;
            ex.instance_name = mTag.empty() ? mObjectName : mObjectName + ":" + mTag;
            ex.plugin_name = mPluginName;
            throw ex;
        }
        catch (...) {
            auto ex = JException("Unknown exception");
            ex.exception_type = JTypeInfo::demangle_current_exception_type();
            ex.nested_exception = std::current_exception();
            ex.function_name = "JFactory::Init";
            ex.type_name = mFactoryName;
            ex.instance_name = mTag.empty() ? mObjectName : mObjectName + ":" + mTag;
            ex.plugin_name = mPluginName;
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
            std::string instanceName = mTag.empty() ? mObjectName : mObjectName + ":" + mTag;
            if (ex.function_name.empty()) ex.function_name = "JFactory::BeginRun/ChangeRun/EndRun(";
            if (ex.type_name.empty()) ex.type_name = mFactoryName;
            if (ex.instance_name.empty()) ex.instance_name = instanceName;
            if (ex.plugin_name.empty()) ex.plugin_name = mPluginName;
            throw ex;
        }
        catch (std::exception& e) {
            auto ex = JException(e.what());
            ex.exception_type = JTypeInfo::demangle_current_exception_type();
            ex.nested_exception = std::current_exception();
            ex.function_name = "JFactory::BeginRun/ChangeRun/EndRun";
            ex.type_name = mFactoryName;
            ex.instance_name = mTag.empty() ? mObjectName : mObjectName + ":" + mTag;
            ex.plugin_name = mPluginName;
            throw ex;
        }
        catch (...) {
            auto ex = JException("Unknown exception");
            ex.exception_type = JTypeInfo::demangle_current_exception_type();
            ex.nested_exception = std::current_exception();
            ex.function_name = "JFactory::BeginRun/ChangeRun/EndRun";
            ex.type_name = mFactoryName;
            ex.instance_name = mTag.empty() ? mObjectName : mObjectName + ":" + mTag;
            ex.plugin_name = mPluginName;
            throw ex;
        }
        try {
            Process(event);
        }
        catch (JException& ex) {
            std::string instanceName = mTag.empty() ? mObjectName : mObjectName + ":" + mTag;
            if (ex.function_name.empty()) ex.function_name = "JFactory::Process";
            if (ex.type_name.empty()) ex.type_name = mFactoryName;
            if (ex.instance_name.empty()) ex.instance_name = instanceName;
            if (ex.plugin_name.empty()) ex.plugin_name = mPluginName;
            throw ex;
        }
        catch (std::exception& e) {
            auto ex = JException(e.what());
            ex.exception_type = JTypeInfo::demangle_current_exception_type();
            ex.nested_exception = std::current_exception();
            ex.function_name = "JFactory::Process";
            ex.type_name = mFactoryName;
            ex.instance_name = mTag.empty() ? mObjectName : mObjectName + ":" + mTag;
            ex.plugin_name = mPluginName;
            throw ex;
        }
        catch (...) {
            auto ex = JException("Unknown exception");
            ex.exception_type = JTypeInfo::demangle_current_exception_type();
            ex.nested_exception = std::current_exception();
            ex.function_name = "JFactory::Process";
            ex.type_name = mFactoryName;
            ex.instance_name = mTag.empty() ? mObjectName : mObjectName + ":" + mTag;
            ex.plugin_name = mPluginName;
            throw ex;
        }
        mStatus = Status::Processed;
        mCreationStatus = CreationStatus::Created;
    }
}
