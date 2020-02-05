//
// Created by Nathan Brei on 2/4/20.
//

#ifndef JANA2_JFACTORYV2_H
#define JANA2_JFACTORYV2_H

#include <JANA/Components/JAbstractFactory.h>
#include <JANA/JLogger.h>

template <typename T>
class JFactoryV2 : public JAbstractFactoryT<T> {
public:

    JFactoryV2(const std::string& factory_name, const std::string& tag="")
    : JFactory(factory_name, tag), JAbstractFactoryT<T>(factory_name, tag)(
            <>) {}

    // -----------------------------
    // Meant to be overriden by user
    // -----------------------------

    virtual void Init() {}

    virtual void ChangeRun(const std::shared_ptr<const JEvent>& aEvent) {}

    virtual void Process(const std::shared_ptr<const JEvent>& aEvent) {
        LOG << "Dummy factory created but nothing was Inserted() or Set()." << LOG_END;
    }

    // -----------------------------------------------------------------------
    // Meant to be called by user (or by JANA, in the case of dummy factories)
    // -----------------------------------------------------------------------

    /// Please use the typed setters instead whenever possible
    void Set(std::vector<JObject*>& aData) {
        ClearData();
        for (auto jobj : aData) {
            T* casted = dynamic_cast<T*>(jobj);
            assert(casted != nullptr);
            mData.push_back(casted);
        }
        JFactory::mStatus = JFactory::Status::Inserted;
    }

    /// Please use the typed setters instead whenever possible
    void Insert(JObject* aDatum) {

        assert(JFactory::mStatus == JFactory::Status::Uninitialized ||
               JFactory::mStatus == JFactory::Status::Unprocessed);
        T* casted = dynamic_cast<T*>(aDatum);
        assert(casted != nullptr);
        mData.push_back(casted);
        JFactory::mStatus = JFactory::Status::Inserted;
    }

    void Set(const std::vector<T*>& aData) {
        ClearData();
        mData = aData;
        JFactory::mStatus = JFactory::Status::Inserted;
    }

    void Set(std::vector<T*>&& aData) {
        ClearData();
        mData = std::move(aData);
        JFactory::mStatus = JFactory::Status::Inserted;
    }

    void Insert(T* aDatum) {
        assert(JFactory::mStatus == JFactory::Status::Uninitialized ||
               JFactory::mStatus == JFactory::Status::Unprocessed);
        mData.push_back(aDatum);
        JFactory::mStatus = JFactory::Status::Inserted;
    }


    // -----------------------------------
    // Meant to be used internally by JANA
    // -----------------------------------

    /// GetOrCreate handles all the preconditions and postconditions involved in calling the user-defined Open(),
    /// ChangeRun(), and Process() methods. These include making sure the JFactory JApplication is set, Init() is called
    /// exactly once, exceptions are tagged with the originating plugin and eventsource, ChangeRun() is
    /// called if and only if the run number changes, etc.
    typename JAbstractFactoryT<T>::PairType GetOrCreate(const std::shared_ptr<const JEvent>& event, JApplication* app, uint64_t run_number) final {

        std::lock_guard<std::mutex> lock(mMutex);
        if (JFactory::mApp == nullptr) {
            JFactory::mApp = app;
        }
        switch (JFactory::mStatus) {
            case JFactory::Status::Uninitialized:
                try {
                    std::call_once(mInitFlag, &JFactoryV2::Init, this);
                }
                catch (JException& ex) {
                    ex.plugin_name = JFactory::mPluginName;
                    ex.component_name = JFactory::mFactoryName;
                    throw ex;
                }
                catch (...) {
                    auto ex = JException("Unknown exception in JFactoryT::Init()");
                    ex.nested_exception = std::current_exception();
                    ex.plugin_name = JFactory::mPluginName;
                    ex.component_name = JFactory::mFactoryName;
                    throw ex;
                }
            case JFactory::Status::Unprocessed:
                if (JFactory::mPreviousRunNumber != run_number) {
                    ChangeRun(event);
                    JFactory::mPreviousRunNumber = run_number;
                }
                Process(event);
                JFactory::mStatus = JFactory::Status::Processed;
            case JFactory::Status::Processed:
            case JFactory::Status::Inserted:
                return std::make_pair(mData.cbegin(), mData.cend());
            default:
                throw JException("Enum is set to a garbage value somehow");
        }
    }


    void ClearData() final {

        // ClearData won't do anything if Init() hasn't been called
        if (JFactory::mStatus == JFactory::Status::Uninitialized) {
            return;
        }
        // ClearData() does nothing if persistent flag is set.
        // User must manually recycle data, e.g. during ChangeRun()
        if (JFactory::TestFactoryFlag(JFactory::JFactory_Flags_t::PERSISTENT)) {
            return;
        }

        // Assuming we _are_ the object owner, delete the underlying jobjects
        if (!JFactory::TestFactoryFlag(JFactory::JFactory_Flags_t::NOT_OBJECT_OWNER)) {
            for (auto p : mData) delete p;
        }
        mData.clear();
        JFactory::mStatus = JFactory::Status::Unprocessed;
    }

private:
    std::vector<T*> mData;
    std::once_flag mInitFlag;
    std::mutex mMutex;
};


#endif //JANA2_JFACTORYV2_H
