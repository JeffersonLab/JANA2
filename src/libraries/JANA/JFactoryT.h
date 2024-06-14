
// Copyright 2020, Jefferson Science Associates, LLC.
// Subject to the terms in the LICENSE file found in the top-level directory.

#pragma once

#include <vector>
#include <type_traits>

#include <JANA/JApplication.h>
#include <JANA/JFactory.h>
#include <JANA/JObject.h>
#include <JANA/Utils/JTypeInfo.h>

#include <JANA/Omni/JBasicCollection.h>

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
    enum JFactory_Flags_t {
        JFACTORY_NULL = 0x00,    // Not used anywhere
        PERSISTENT = 0x01,       // Used heavily. Possibly better served by JServices, hierarchical events, or event groups. 
        WRITE_TO_OUTPUT = 0x02,  // Set in halld_recon but not read except by JANA1 janaroot and janacontrol plugins
        NOT_OBJECT_OWNER = 0x04, // Used heavily. Small conflict with PODIO subset collections, which do the same thing at a different level
        REGENERATE = 0x08        // Replaces JANA1 JFactory_base::use_factory and JFactory::GetCheckSourceFirst()
    };

protected:
    // Fields
    JBasicCollectionT<T>* mCollection;
    JMetadata<T> mMetadata;
    std::vector<T*> mData;

public:
    using IteratorType = typename std::vector<T*>::const_iterator;
    using PairType = std::pair<IteratorType, IteratorType>;

    /// JFactoryT constructor requires a name and a tag.
    /// Name should always be JTypeInfo::demangle<T>(), tag is usually "".
    JFactoryT(const std::string& aName, const std::string& aTag) __attribute__ ((deprecated)) : JFactory(aName, aTag) {
        mCollection = new JBasicCollectionT<T>();
        mCollection->SetCollectionTag(aTag);
        mCollection->template EnableGetAs<T>();
        mCollection->template EnableGetAs<JObject>( std::is_convertible<T,JObject>() ); // Automatically add JObject if this can be converted to it
#ifdef JANA2_HAVE_ROOT
        mCollection->template EnableGetAs<TObject>( std::is_convertible<T,TObject>() ); // Automatically add TObject if this can be converted to it
#endif
        mCollections.push_back(mCollection);
    }

    JFactoryT(const std::string& aName) __attribute__ ((deprecated))  : JFactory(aName, "") {
        mCollection = new JBasicCollectionT<T>();
        mCollection->template EnableGetAs<T>();
        mCollection->template EnableGetAs<JObject>( std::is_convertible<T,JObject>() ); // Automatically add JObject if this can be converted to it
#ifdef JANA2_HAVE_ROOT
        mCollection->template EnableGetAs<TObject>( std::is_convertible<T,TObject>() ); // Automatically add TObject if this can be converted to it
#endif
        mCollections.push_back(mCollection);
    }

    JFactoryT() : JFactory(JTypeInfo::demangle<T>(), ""){
        mCollection = new JBasicCollectionT<T>();
        mCollection->template EnableGetAs<T>();
        mCollection->template EnableGetAs<JObject>( std::is_convertible<T,JObject>() ); // Automatically add JObject if this can be converted to it
#ifdef JANA2_HAVE_ROOT
        mCollection->template EnableGetAs<TObject>( std::is_convertible<T,TObject>() ); // Automatically add TObject if this can be converted to it
#endif
        mCollections.push_back(mCollection);
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
        return mCollection->GetSize();
    }

    /// CreateAndGetData handles all the preconditions and postconditions involved in calling the user-defined Open(),
    /// ChangeRun(), and Process() methods. These include making sure the JFactory JApplication is set, Init() is called
    /// exactly once, exceptions are tagged with the originating plugin and eventsource, ChangeRun() is
    /// called if and only if the run number changes, etc.
    PairType CreateAndGetData(const std::shared_ptr<const JEvent>& event) {
        Create(event);
        auto& mData = mCollection->GetData();
        return std::make_pair(mData.cbegin(), mData.cend());
    }


    /// Please use the typed setters instead whenever possible
    // TODO: Deprecate this!
    void Set(const std::vector<JObject*>& aData) override {
        mCollection->ClearData();
        std::vector<T*>& data = mCollection->GetData();
        for (auto obj : aData) {
            T* casted = dynamic_cast<T*>(obj);
            assert(casted != nullptr);
            data.push_back(casted);
        }
    }

    /// Please use the typed setters instead whenever possible
    // TODO: Deprecate this!
    void Insert(JObject* aDatum) override {
        T* casted = dynamic_cast<T*>(aDatum);
        assert(casted != nullptr);
        Insert(casted);
    }

    virtual void Set(const std::vector<T*>& aData) {
        mCollection->ClearData();
        mCollection->GetData() = aData;
        mCollection->SetCreationStatus(JCollection::CreationStatus::Inserted);
        mStatus = Status::Inserted;
    }

    virtual void Set(std::vector<T*>&& aData) {
        mCollection->ClearData();
        auto& data = mCollection->GetData();
        data = std::move(aData);
        mCollection->SetCreationStatus(JCollection::CreationStatus::Inserted);
        mStatus = Status::Inserted;
    }

    virtual void Insert(T* aDatum) {
        auto& data = mCollection->GetData();
        data.push_back(aDatum);
        mCollection->SetCreationStatus(JCollection::CreationStatus::Inserted);
        mStatus = Status::Inserted;
    }


    /// EnableGetAs generates a vtable entry so that users may extract the
    /// contents of this JFactoryT from the type-erased JFactory. The user has to manually specify which upcasts
    /// to allow, and they have to do so for each instance. It is recommended to do so in the constructor.
    /// Note that EnableGetAs<T>() is called automatically.
    template <typename S> void EnableGetAs() { mCollection->template EnableGetAs<S>(); }

    // The following specializations allow automatically adding standard types (e.g. JObject) using things like
    // std::is_convertible(). The std::true_type version defers to the standard EnableGetAs().
    template <typename S> void EnableGetAs(std::true_type) { EnableGetAs<S>(); }
    template <typename S> void EnableGetAs(std::false_type) {}

    void ClearData() override {

        // ClearData won't do anything if Init() hasn't been called
        if (mStatus == Status::Uninitialized) {
            return;
        }
        mCollection->ClearData();
        mStatus = Status::Unprocessed;
    }

    /// Set the JFactory's metadata. This is meant to be called by user during their JFactoryT::Process
    /// Metadata will *not* be cleared on ClearData(), but will be destroyed when the JFactoryT is.
    void SetMetadata(JMetadata<T> metadata) { mMetadata = metadata; }

    /// Get the JFactory's metadata. This is meant to be called by user during their JFactoryT::Process
    /// and also used by JEvent under the hood.
    /// Metadata will *not* be cleared on ClearData(), but will be destroyed when the JFactoryT is.
    JMetadata<T> GetMetadata() { return mMetadata; }


    // Note: JFactory_Flags_t is going to be deprecated. Use Set...Flag()s instead
    /// Get all flags in the form of a single word
    inline uint32_t GetFactoryFlags() const { 
        uint32_t mFlags = 0;
        if (mCollection->GetPersistentFlag()) { mFlags += 1; }
        if (mCollection->GetWriteToOutputFlag()) { mFlags += 2; }
        if (mCollection->GetNotOwnerFlag()) { mFlags += 4; }
        if (GetRegenerateFlag()) { mFlags += 8; }
        return mFlags; 
    }

    /// Set a flag (or flags)
    inline void SetFactoryFlag(JFactory_Flags_t f) {
        switch (f) {
            case PERSISTENT: return mCollection->SetPersistentFlag(true);
            case NOT_OBJECT_OWNER: return mCollection->SetNotOwnerFlag(true);
            case REGENERATE: return SetRegenerateFlag(true);
            case WRITE_TO_OUTPUT: return mCollection->SetWriteToOutputFlag(true);
            default: throw JException("Invalid factory flag %d", f);
        }
    }

    /// Clear a flag (or flags)
    inline void ClearFactoryFlag(JFactory_Flags_t f) {
        switch (f) {
            case PERSISTENT: return mCollection->SetPersistentFlag(false);
            case NOT_OBJECT_OWNER: return mCollection->SetNotOwnerFlag(false);
            case REGENERATE: return SetRegenerateFlag(false);
            case WRITE_TO_OUTPUT: return mCollection->SetWriteToOutputFlag(false);
            default: throw JException("Invalid factory flag %d", f);
        }
    }

    /// Test if a flag (or set of flags) is set
    inline bool TestFactoryFlag(JFactory_Flags_t f) const {
        switch (f) {
            case PERSISTENT: return mCollection->GetPersistentFlag();
            case NOT_OBJECT_OWNER: return mCollection->GetNotOwnerFlag();
            case REGENERATE: return GetRegenerateFlag();
            case WRITE_TO_OUTPUT: return mCollection->GetWriteToOutputFlag();
            default: throw JException("Invalid factory flag %d", f);
        }
    }

    inline void SetPersistentFlag(bool persistent) { 
        mCollection->SetPersistentFlag(persistent);
    }

    inline void SetNotOwnerFlag(bool not_owner) { 
        mCollection->SetNotOwnerFlag(not_owner);
    }

    inline void SetWriteToOutputFlag(bool write_to_output) { 
        mCollection->SetWriteToOutputFlag(write_to_output);
    }

    inline bool GetWriteToOutputFlag() { 
        return mCollection->GetWriteToOutputFlag();
    }


};



