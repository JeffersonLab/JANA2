
// Copyright 2020, Jefferson Science Associates, LLC.
// Subject to the terms in the LICENSE file found in the top-level directory.

#include <vector>

#include <JANA/JApplication.h>
#include <JANA/JFactory.h>
#include <JANA/JObject.h>
#include <JANA/Utils/JTypeInfo.h>

#ifndef _JFactoryT_h_
#define _JFactoryT_h_

/// Class template for metadata. This constrains JFactoryT<T> to use the same (user-defined)
/// metadata structure, JMetadata<T> for that T. This is essential for retrieving metadata from
/// JFactoryT's without breaking the Liskov substitution property.
template<typename T>
struct JMetadata {};

template<typename T>
class JFactoryT : public JFactory {
public:

    using IteratorType = typename std::vector<T*>::const_iterator;
    using PairType = std::pair<IteratorType, IteratorType>;


    JFactoryT(const std::string& aName = JTypeInfo::demangle<T>(), const std::string& aTag = "")
    : JFactory(aName, aTag) {
        EnableGetAs<T>();
    }

    ~JFactoryT() override = default;


    void Init() override {}
    void ChangeRun(const std::shared_ptr<const JEvent>& aEvent) override {}
    void Process(const std::shared_ptr<const JEvent>& aEvent) override {
        // TODO: Debate best thing to do in this case. Consider fa250WaveboardV1Hit
        LOG << "Dummy factory created but nothing was Inserted() or Set()." << LOG_END;
        //throw JException("Dummy factory created but nothing was Inserted() or Set().");
    }


    std::type_index GetObjectType(void) const override {
        return std::type_index(typeid(T));
    }

    /// GetOrCreate handles all the preconditions and postconditions involved in calling the user-defined Open(),
    /// ChangeRun(), and Process() methods. These include making sure the JFactory JApplication is set, Init() is called
    /// exactly once, exceptions are tagged with the originating plugin and eventsource, ChangeRun() is
    /// called if and only if the run number changes, etc.
    PairType GetOrCreate(const std::shared_ptr<const JEvent>& event, JApplication* app, uint64_t run_number) {

        //std::lock_guard<std::mutex> lock(mMutex);
        if (mApp == nullptr) {
            mApp = app;
        }
        switch (mStatus) {
            case Status::Uninitialized:
                try {
                    std::call_once(mInitFlag, &JFactory::Init, this);
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
            case Status::Unprocessed:
                if (mPreviousRunNumber != run_number) {
                    ChangeRun(event);
                    mPreviousRunNumber = run_number;
                }
                Process(event);
                mStatus = Status::Processed;
            case Status::Processed:
            case Status::Inserted:
                return std::make_pair(mData.cbegin(), mData.cend());
            default:
                throw JException("Enum is set to a garbage value somehow");
        }
    }

    /// Please use the typed setters instead whenever possible
    void Set(std::vector<JObject*>& aData) override {
        ClearData();
        for (auto jobj : aData) {
            T* casted = dynamic_cast<T*>(jobj);
            assert(casted != nullptr);
            mData.push_back(casted);
        }
    }

    /// Please use the typed setters instead whenever possible
    void Insert(JObject* aDatum) override {
        T* casted = dynamic_cast<T*>(aDatum);
        assert(casted != nullptr);
        mData.push_back(casted);
        mStatus = Status::Inserted;
        // TODO: assert correct mStatus precondition
    }

    void Set(const std::vector<T*>& aData) {
        ClearData();
        mData = aData;
        mStatus = Status::Inserted;
    }

    void Set(std::vector<T*>&& aData) {
        ClearData();
        mData = std::move(aData);
        mStatus = Status::Inserted;
    }

    void Insert(T* aDatum) {
        mData.push_back(aDatum);
        mStatus = Status::Inserted;
    }


    /// EnableGetAs generates a vtable entry so that users may extract the
    /// contents of this JFactoryT from the type-erased JFactory. The user has to manually specify which upcasts
    /// to allow, and they have to do so for each instance. It is recommended to do so in the constructor.
    /// Note that EnableGetAs<T>() is called automatically.
    template <typename S> void EnableGetAs ();


    void ClearData() override {

        // ClearData won't do anything if Init() hasn't been called
        if (mStatus == Status::Uninitialized) {
            return;
        }
        // ClearData() does nothing if persistent flag is set.
        // User must manually recycle data, e.g. during ChangeRun()
        if (TestFactoryFlag(JFactory_Flags_t::PERSISTENT)) {
            return;
        }

        // Assuming we _are_ the object owner, delete the underlying jobjects
        if (!TestFactoryFlag(JFactory_Flags_t::NOT_OBJECT_OWNER)) {
            for (auto p : mData) delete p;
        }
        mData.clear();
        mStatus = Status::Unprocessed;
    }

    /// Set the JFactory's metadata. This is meant to be called by user during their JFactoryT::Process
    /// Metadata will *not* be cleared on ClearData(), but will be destroyed when the JFactoryT is.
    void SetMetadata(JMetadata<T> metadata) { mMetadata = metadata; }

    /// Get the JFactory's metadata. This is meant to be called by user during their JFactoryT::Process
    /// and also used by JEvent under the hood.
    /// Metadata will *not* be cleared on ClearData(), but will be destroyed when the JFactoryT is.
    JMetadata<T> GetMetadata() { return mMetadata; }


protected:
    std::vector<T*> mData;
    JMetadata<T> mMetadata;
};

template<typename T>
template<typename S>
void JFactoryT<T>::EnableGetAs() {

    auto upcast_lambda = [this]() {
        std::vector<S*> results;
        for (auto t : mData) {
            results.push_back(static_cast<S*>(t));
        }
        return results;
    };

    auto key = std::type_index(typeid(S));
    using upcast_fn_t = std::function<std::vector<S*>()>;
    auto upcast_fn = new upcast_fn_t(upcast_lambda);
    mUpcastVTable[key] = upcast_fn;
}

#endif // _JFactoryT_h_

