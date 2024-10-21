
#pragma once

#include <JANA/Components/JDataBundle.h>
#include <JANA/JObject.h> 
#include <JANA/Utils/JTypeInfo.h>

#ifdef JANA2_HAVE_ROOT
#include <TObject.h>
#endif

class JBasicDataBundle : public JDataBundle {
    bool m_is_persistent = false;
    bool m_not_object_owner = false;
    bool m_write_to_output = true;

public:
    JBasicDataBundle() = default;
    ~JBasicDataBundle() override = default;
    void SetPersistentFlag(bool persistent) { m_is_persistent = persistent; }
    void SetNotOwnerFlag(bool not_owner) { m_not_object_owner = not_owner; }
    void SetWriteToOutputFlag(bool write_to_output) { m_write_to_output = write_to_output; }

    bool GetPersistentFlag() { return m_is_persistent; }
    bool GetNotOwnerFlag() { return m_not_object_owner; }
    bool GetWriteToOutputFlag() { return m_write_to_output; }
};



template <typename T>
class JBasicDataBundleT : public JBasicDataBundle {
private:
    std::vector<T*> m_data;

public:
    JBasicDataBundleT();
    void ClearData() override;
    size_t GetSize() const override { return m_data.size();}

    std::vector<T*>& GetData() { return m_data; }

    /// EnableGetAs generates a vtable entry so that users may extract the
    /// contents of this JFactoryT from the type-erased JFactory. The user has to manually specify which upcasts
    /// to allow, and they have to do so for each instance. It is recommended to do so in the constructor.
    /// Note that EnableGetAs<T>() is called automatically.
    template <typename S> void EnableGetAs ();

    // The following specializations allow automatically adding standard types (e.g. JObject) using things like
    // std::is_convertible(). The std::true_type version defers to the standard EnableGetAs().
    template <typename S> void EnableGetAs(std::true_type) { EnableGetAs<S>(); }
    template <typename S> void EnableGetAs(std::false_type) {}
};

// Template definitions

template <typename T>
JBasicDataBundleT<T>::JBasicDataBundleT() {
    SetTypeName(JTypeInfo::demangle<T>());
    EnableGetAs<T>();
    EnableGetAs<JObject>( std::is_convertible<T,JObject>() ); // Automatically add JObject if this can be converted to it
#ifdef JANA2_HAVE_ROOT
    EnableGetAs<TObject>( std::is_convertible<T,TObject>() ); // Automatically add TObject if this can be converted to it
#endif
}

template <typename T>
void JBasicDataBundleT<T>::ClearData() {

    // ClearData won't do anything if Init() hasn't been called
    if (GetStatus() == Status::Empty) {
        return;
    }
    // ClearData() does nothing if persistent flag is set.
    // User must manually recycle data, e.g. during ChangeRun()
    if (GetPersistentFlag()) {
        return;
    }

    // Assuming we _are_ the object owner, delete the underlying jobjects
    if (!GetNotOwnerFlag()) {
        for (auto p : m_data) delete p;
    }
    m_data.clear();
    SetStatus(Status::Empty);
}

template<typename T>
template<typename S>
void JBasicDataBundleT<T>::EnableGetAs() {

    auto upcast_lambda = [this]() {
        std::vector<S*> results;
        for (auto t : m_data) {
            results.push_back(static_cast<S*>(t));
        }
        return results;
    };

    auto key = std::type_index(typeid(S));
    using upcast_fn_t = std::function<std::vector<S*>()>;
    mUpcastVTable[key] = std::unique_ptr<JAny>(new JAnyT<upcast_fn_t>(std::move(upcast_lambda)));
}





