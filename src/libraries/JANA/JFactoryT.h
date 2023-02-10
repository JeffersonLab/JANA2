
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

#ifdef HAVE_ROOT
#include <TObject.h>
#endif

#ifdef HAVE_PODIO
#include <podio/Frame.h>
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
#ifdef HAVE_ROOT
        EnableGetAs<TObject>( std::is_convertible<T,TObject>() ); // Automatically add TObject if this can be converted to it
#endif
    }

    JFactoryT(const std::string& aName) __attribute__ ((deprecated))  : JFactory(aName, "") {
        EnableGetAs<T>();
        EnableGetAs<JObject>( std::is_convertible<T,JObject>() ); // Automatically add JObject if this can be converted to it
#ifdef HAVE_ROOT
        EnableGetAs<TObject>( std::is_convertible<T,TObject>() ); // Automatically add TObject if this can be converted to it
#endif
    }

    JFactoryT() : JFactory(JTypeInfo::demangle<T>(), ""){
        EnableGetAs<T>();
        EnableGetAs<JObject>( std::is_convertible<T,JObject>() ); // Automatically add JObject if this can be converted to it
#ifdef HAVE_ROOT
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

    /// GetOrCreate handles all the preconditions and postconditions involved in calling the user-defined Open(),
    /// ChangeRun(), and Process() methods. These include making sure the JFactory JApplication is set, Init() is called
    /// exactly once, exceptions are tagged with the originating plugin and eventsource, ChangeRun() is
    /// called if and only if the run number changes, etc.
    PairType GetOrCreate(const std::shared_ptr<const JEvent>& event) {
        if (mStatus == Status::Uninitialized || mStatus == Status::Unprocessed) {
            Create(event);
        }
        if (mStatus != Status::Processed && mStatus != Status::Inserted) {
            throw JException("JFactoryT::Status is corrupted");
        }
        return std::make_pair(mData.cbegin(), mData.cend());
    }

    /// Please use the typed setters instead whenever possible
    void Set(const std::vector<JObject*>& aData) override {
        ClearData();
        if (mIsPodio) {
#ifdef HAVE_PODIO
            auto coll = std::make_unique<typename PodioTypeMap<T>::collection_t>();
            for (auto jobj: aData) {
                T *casted = dynamic_cast<T *>(jobj);
                assert(casted != nullptr);
                coll->push_back(*casted);
            }
            auto moved = mFrame->put(coll, mTag);
            mCollection = &moved;
            for (const T &item: moved) {
                mData.push_back(&item);
            }
#else
        throw std::runtime_error("Not compiled with Podio support! mIsPodio is probably corrupted");
#endif
        }
        else {
            for (auto jobj: aData) {
                T *casted = dynamic_cast<T *>(jobj);
                assert(casted != nullptr);
                mData.push_back(casted);
            }
        }
        mStatus = Status::Inserted;                  // n.b. This will be overwritten in GetOrCreate above
        mCreationStatus = CreationStatus::Inserted;  // n.b. This will be overwritten in GetOrCreate above
    }

    /// Please use the typed setters instead whenever possible
    void Insert(JObject* aDatum) override {
        if (mIsPodio) {
#ifdef HAVE_PODIO
            T *casted = dynamic_cast<T *>(aDatum);
            assert(casted != nullptr);
            auto coll = std::make_unique<typename PodioTypeMap<T>::collection_t>();
            coll->push_back(casted);
            auto moved = mFrame->put(coll, mTag);
            mCollection = &moved;
            mData.push_back(moved[0]);
#else
            throw std::runtime_error("Not compiled with Podio support! mIsPodio is probably corrupted");
#endif
        }
        else {
            T *casted = dynamic_cast<T *>(aDatum);
            assert(casted != nullptr);
            mData.push_back(casted);
        }
        mStatus = Status::Inserted;
        mCreationStatus = CreationStatus::Inserted;
    }

    void Set(const std::vector<T*>& aData) {
        // If the user populates mData directly instead of populating a temporary vector, they still need to
        // call Set() so that the JFactory::Status gets updated.
        // Doing this breaks the JFactory::Status invariant unless they remember to call Set() afterwards.
        // Ideally, they would use a temporary vector and not access mData at all, but they are used to this
        // from JANA1 and I haven't found a cleaner solution that gives them what they want yet.

        if (mIsPodio) {
#ifdef HAVE_PODIO
            if (aData != mData) {
                ClearData();
                mData = aData;
                auto coll = new typename PodioTypeMap<T>::collection_t;
                for (T* d : aData) {
                    coll->push_back(*d);
                }
                mCollection = &(mFrame->put(coll, mTag));
            }
#else
            throw std::runtime_error("Not compiled with Podio support! mIsPodio is probably corrupted");
#endif
        }
        else {
            if (aData != mData) {
                ClearData();
                mData = aData;
            }
        }
        mStatus = Status::Inserted;
        mCreationStatus = CreationStatus::Inserted;
    }

    void Set(std::vector<T*>&& aData) {
        if (mIsPodio) {
#ifdef HAVE_PODIO
            auto coll = new typename PodioTypeMap<T>::collection_t;
            for (T *d: aData) {
                coll->push_back(d);
            }
            auto moved = mFrame->put(coll, mTag);
            mCollection = &moved;
            for (const T &item: moved) {
                mData.push_back(&item);
            }
#else
            throw std::runtime_error("Not compiled with Podio support! mIsPodio is probably corrupted");
#endif
        }
        else {
            ClearData();
            mData = std::move(aData);
        }
        mStatus = Status::Inserted;
        mCreationStatus = CreationStatus::Inserted;
    }

    void Insert(T* aDatum) {
        if (mIsPodio) {
#ifdef HAVE_PODIO
            auto coll = new typename PodioTypeMap<T>::collection_t;
            coll->push_back(aDatum);
            typename PodioTypeMap<T>::collection_t *moved = &(mFrame->put(coll, mTag));
            mCollection = moved;
            mData.push_back(&mCollection[0]);
#else
            throw std::runtime_error("Not compiled with Podio support! mIsPodio is probably corrupted");
#endif
        }
        else {
            mData.push_back(aDatum);
        }
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
        mCollection = nullptr;  // "Cleared" when frame gets cleared
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


#ifdef HAVE_PODIO
    void EnablePodioCollectionAccess() {
        mIsPodio = true;
        // Factory IS the owner of the mData pointers, because those are just lightweight
        // wrapper objects that point into the Collection.
    }

    template <typename S> struct PodioTypeMap;

    const podio::CollectionBase* GetCollection() { return mCollection; } // TODO: GetOrCreate?

    template <typename PodioT> void SetCollection(std::unique_ptr<typename PodioTypeMap<PodioT>::collection_t> collection) {

        if (!mIsPodio) throw std::runtime_error("PODIO collection access has not been enabled!");
        auto moved = mFrame->put(collection, mTag);
        mCollection = &moved;
        for (const PodioT& item : moved) {
            mData.push_back(&item);
        }
        mStatus = Status::Inserted;
        mCreationStatus = CreationStatus::Inserted;
    }

#endif



protected:
    std::vector<T*> mData;
    JMetadata<T> mMetadata;
    bool mIsPodio = false; // Always false unless user chooses to EnablePodioCollectionAccess()

#ifdef HAVE_PODIO
    const podio::CollectionBase* mCollection = nullptr;
    // mCollection is owned by the frame.
    // mFrame is owned by the JFactoryT<podio::Frame>.
    // mData holds lightweight value objects which hold a pointer into mCollection.
    // This factory owns these value objects.
#endif
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

