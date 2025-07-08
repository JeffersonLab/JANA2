
// Copyright 2020, Jefferson Science Associates, LLC.
// Subject to the terms in the LICENSE file found in the top-level directory.

#pragma once

#include <vector>
#include <type_traits>

#include <JANA/JApplication.h>
#include <JANA/JFactory.h>
#include <JANA/JObject.h>
#include <JANA/JVersion.h>
#include <JANA/Utils/JTypeInfo.h>
#include <JANA/Components/JLightweightOutput.h>

#if JANA2_HAVE_ROOT
#include <TObject.h>
#endif


template<typename T>
class JFactoryT : public JFactory {
public:

    using IteratorType = typename std::vector<T*>::const_iterator;
    using PairType = std::pair<IteratorType, IteratorType>;

    JFactoryT(std::string tag="") {
        mOutput.GetDatabundle().AttachData(&mData);
        SetTag(tag);
        SetPrefix(mOutput.GetDatabundle().GetUniqueName());
        SetObjectName(mOutput.GetDatabundle().GetTypeName());

        EnableGetAs<T>();
        EnableGetAs<JObject>( std::is_convertible<T,JObject>() ); // Automatically add JObject if this can be converted to it
#if JANA2_HAVE_ROOT
        EnableGetAs<TObject>( std::is_convertible<T,TObject>() ); // Automatically add TObject if this can be converted to it
#endif
    }

    ~JFactoryT() override = default;

    void Init() override {}
    void BeginRun(const std::shared_ptr<const JEvent>&) override {}
    void ChangeRun(const std::shared_ptr<const JEvent>&) override {}
    void EndRun() override {}
    void Process(const std::shared_ptr<const JEvent>&) override {}


    /// CreateAndGetData handles all the preconditions and postconditions involved in calling the user-defined Open(),
    /// ChangeRun(), and Process() methods. These include making sure the JFactory JApplication is set, Init() is called
    /// exactly once, exceptions are tagged with the originating plugin and eventsource, ChangeRun() is
    /// called if and only if the run number changes, etc.
    PairType CreateAndGetData(const std::shared_ptr<const JEvent>& event) {
        Create(*event.get());
        return std::make_pair(mData.cbegin(), mData.cend());
    }

    PairType CreateAndGetData(const JEvent& event) {
        Create(event);
        return std::make_pair(mData.cbegin(), mData.cend());
    }

    // Retrieve a const reference to the data directly (no copying!)
    const std::vector<T*>& GetData() {
        return mData;
    }

    /// Please use the typed setters instead whenever possible
    [[deprecated]]
    void Set(const std::vector<JObject*>& aData) override {
        std::vector<T*> data;
        for (auto obj : aData) {
            T* casted = dynamic_cast<T*>(obj);
            assert(casted != nullptr);
            data.push_back(casted);
        }
        Set(std::move(data));
    }

    virtual void Set(const std::vector<T*>& aData) {
        if (aData == mData) {
            // The user populated mData directly instead of populating a temporary vector and passing it to us.
            // Doing this breaks the JFactory::Status invariant unless they remember to call Set() afterwards.
            // Ideally, they would use a temporary vector and not access mData at all, but they are used to this
            // from JANA1 and I haven't found a cleaner solution that gives them what they want yet.
            mStatus = Status::Inserted;
            mCreationStatus = CreationStatus::Inserted;
        }
        else {
            ClearData();
            mData = aData;
            mStatus = Status::Inserted;
            mCreationStatus = CreationStatus::Inserted;
        }
    }

    virtual void Set(std::vector<T*>&& aData) {
        ClearData();
        mData = std::move(aData);
        mStatus = Status::Inserted;
        mCreationStatus = CreationStatus::Inserted;
    }

    virtual void Insert(T* aDatum) {
        mData.push_back(aDatum);
        mStatus = Status::Inserted;
        mCreationStatus = CreationStatus::Inserted;
    }

    /// Set a flag (or flags)
    inline void SetFactoryFlag(JFactory_Flags_t f) override {
        switch (f) {
            case JFactory::PERSISTENT: SetPersistentFlag(true); break;
            case JFactory::NOT_OBJECT_OWNER: SetNotOwnerFlag(true); break;
            case JFactory::REGENERATE: SetRegenerateFlag(true); break;
            case JFactory::WRITE_TO_OUTPUT: SetWriteToOutputFlag(true); break;
            default: throw JException("Invalid factory flag");
        }
    }

    /// Clear a flag (or flags)
    inline void ClearFactoryFlag(JFactory_Flags_t f) {
        switch (f) {
            case JFactory::PERSISTENT: SetPersistentFlag(false); break;
            case JFactory::NOT_OBJECT_OWNER: SetNotOwnerFlag(false); break;
            case JFactory::REGENERATE: SetRegenerateFlag(false); break;
            case JFactory::WRITE_TO_OUTPUT: SetWriteToOutputFlag(false); break;
            default: throw JException("Invalid factory flag");
        }
    }

    inline void SetNotOwnerFlag(bool not_owner) {
        mOutput.GetDatabundle().SetNotOwnerFlag(not_owner);
    }


    /// EnableGetAs generates a vtable entry so that users may extract the
    /// contents of this JFactoryT from the type-erased JFactory. The user has to manually specify which upcasts
    /// to allow, and they have to do so for each instance. It is recommended to do so in the constructor.
    /// Note that EnableGetAs<T>() is called automatically.
    template <typename S> void EnableGetAs ();

    // The following specializations allow automatically adding standard types (e.g. JObject) using things like
    // std::is_convertible(). The std::true_type version defers to the standard EnableGetAs().
    template <typename S> void EnableGetAs(std::true_type) { EnableGetAs<S>(); }
    template <typename S> void EnableGetAs(std::false_type) {}

protected:
    jana::components::Output<T> mOutput {this};
    std::vector<T*> mData;
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
    mUpcastVTable[key] = std::unique_ptr<JAny>(new JAnyT<upcast_fn_t>(std::move(upcast_lambda)));
}


