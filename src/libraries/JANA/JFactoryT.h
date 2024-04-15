
// Copyright 2020, Jefferson Science Associates, LLC.
// Subject to the terms in the LICENSE file found in the top-level directory.

#ifndef _JFactoryT_h_
#define _JFactoryT_h_

#include <vector>
#include <type_traits>

#include <JANA/JApplication.h>
#include <JANA/JFactory.h>
#include <JANA/JObject.h>
#include <JANA/Utils/JTypeInfo.h>

#ifdef JANA2_HAVE_ROOT
#include <TObject.h>
#endif


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

    /// JFactoryT constructor requires a name and a tag.
    /// Name should always be JTypeInfo::demangle<T>(), tag is usually "".
    JFactoryT(const std::string& aName, const std::string& aTag) __attribute__ ((deprecated)) : JFactory(aName, aTag) {
        EnableGetAs<T>();
        EnableGetAs<JObject>( std::is_convertible<T,JObject>() ); // Automatically add JObject if this can be converted to it
#ifdef JANA2_HAVE_ROOT
        EnableGetAs<TObject>( std::is_convertible<T,TObject>() ); // Automatically add TObject if this can be converted to it
#endif
    }

    JFactoryT(const std::string& aName) __attribute__ ((deprecated))  : JFactory(aName, "") {
        EnableGetAs<T>();
        EnableGetAs<JObject>( std::is_convertible<T,JObject>() ); // Automatically add JObject if this can be converted to it
#ifdef JANA2_HAVE_ROOT
        EnableGetAs<TObject>( std::is_convertible<T,TObject>() ); // Automatically add TObject if this can be converted to it
#endif
    }

    JFactoryT() : JFactory(JTypeInfo::demangle<T>(), ""){
        EnableGetAs<T>();
        EnableGetAs<JObject>( std::is_convertible<T,JObject>() ); // Automatically add JObject if this can be converted to it
#ifdef JANA2_HAVE_ROOT
        EnableGetAs<TObject>( std::is_convertible<T,TObject>() ); // Automatically add TObject if this can be converted to it
#endif
    }

    ~JFactoryT() override = default;

    void Init() override {}
    void BeginRun(const std::shared_ptr<const JEvent>&) override {}
    void ChangeRun(const std::shared_ptr<const JEvent>&) override {}
    void EndRun() override {}
    void Process(const std::shared_ptr<const JEvent>&) override {}


    std::type_index GetObjectType(void) const override {
        return std::type_index(typeid(T));
    }

    std::size_t GetNumObjects(void) const override {
        return mData.size();
    }

    /// CreateAndGetData handles all the preconditions and postconditions involved in calling the user-defined Open(),
    /// ChangeRun(), and Process() methods. These include making sure the JFactory JApplication is set, Init() is called
    /// exactly once, exceptions are tagged with the originating plugin and eventsource, ChangeRun() is
    /// called if and only if the run number changes, etc.
    PairType CreateAndGetData(const std::shared_ptr<const JEvent>& event) {
        Create(event);
        return std::make_pair(mData.cbegin(), mData.cend());
    }


    /// Please use the typed setters instead whenever possible
    // TODO: Deprecate this!
    void Set(const std::vector<JObject*>& aData) override {
        std::vector<T*> data;
        for (auto obj : aData) {
            T* casted = dynamic_cast<T*>(obj);
            assert(casted != nullptr);
            data.push_back(casted);
        }
        Set(std::move(data));
    }

    /// Please use the typed setters instead whenever possible
    // TODO: Deprecate this!
    void Insert(JObject* aDatum) override {
        T* casted = dynamic_cast<T*>(aDatum);
        assert(casted != nullptr);
        Insert(casted);
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


    /// EnableGetAs generates a vtable entry so that users may extract the
    /// contents of this JFactoryT from the type-erased JFactory. The user has to manually specify which upcasts
    /// to allow, and they have to do so for each instance. It is recommended to do so in the constructor.
    /// Note that EnableGetAs<T>() is called automatically.
    template <typename S> void EnableGetAs ();

    // The following specializations allow automatically adding standard types (e.g. JObject) using things like
    // std::is_convertible(). The std::true_type version defers to the standard EnableGetAs().
    template <typename S> void EnableGetAs(std::true_type) { EnableGetAs<S>(); }
    template <typename S> void EnableGetAs(std::false_type) {}

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
        mCreationStatus = CreationStatus::NotCreatedYet;
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
    mUpcastVTable[key] = std::unique_ptr<JAny>(new JAnyT<upcast_fn_t>(std::move(upcast_lambda)));
}

#endif // _JFactoryT_h_

