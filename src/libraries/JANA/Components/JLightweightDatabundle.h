
#pragma once

#include <JANA/Components/JDatabundle.h>
#include <JANA/JObject.h> 
#include <JANA/Utils/JTypeInfo.h>

#if JANA2_HAVE_ROOT
#include <TObject.h>
#endif

template <typename T>
class JLightweightDatabundleT : public JDatabundle {
private:
    std::vector<T*>* m_data = nullptr;
    bool m_is_owned = false;
    bool m_is_persistent = false;
    bool m_not_object_owner = false;

public:
    JLightweightDatabundleT();
    ~JLightweightDatabundleT();

    void AttachData(std::vector<T*>* data) { m_data = data; }
    void UseSelfContainedData() {
        m_data = new std::vector<T*>;
        m_is_owned = true;
    }
    void ClearData() override;

    size_t GetSize() const override { return m_data->size();}
    std::vector<T*>& GetData() { return *m_data; }

    void SetPersistentFlag(bool persistent) { m_is_persistent = persistent; }
    void SetNotOwnerFlag(bool not_owner) { m_not_object_owner = not_owner; }

    bool GetPersistentFlag() { return m_is_persistent; }
    bool GetNotOwnerFlag() { return m_not_object_owner; }

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
JLightweightDatabundleT<T>::JLightweightDatabundleT() {
    SetTypeName(JTypeInfo::demangle<T>());
    EnableGetAs<T>();
    EnableGetAs<JObject>( std::is_convertible<T,JObject>() ); // Automatically add JObject if this can be converted to it
#if JANA2_HAVE_ROOT
    EnableGetAs<TObject>( std::is_convertible<T,TObject>() ); // Automatically add TObject if this can be converted to it
#endif
}

template <typename T>
JLightweightDatabundleT<T>::~JLightweightDatabundleT() {
    if (m_is_owned) {
        delete m_data;
    }
}

template <typename T>
void JLightweightDatabundleT<T>::ClearData() {

    // ClearData() does nothing if persistent flag is set.
    // User must manually recycle data, e.g. during ChangeRun()
    if (GetPersistentFlag()) {
        return;
    }

    // Assuming we _are_ the object owner, delete the underlying jobjects
    if (!GetNotOwnerFlag()) {
        for (auto p : *m_data) delete p;
    }
    m_data->clear();
    SetStatus(Status::Empty);
}

template<typename T>
template<typename S>
void JLightweightDatabundleT<T>::EnableGetAs() {

    auto upcast_lambda = [this]() {
        std::vector<S*> results;
        for (auto t : *m_data) {
            results.push_back(static_cast<S*>(t));
        }
        return results;
    };

    auto key = std::type_index(typeid(S));
    using upcast_fn_t = std::function<std::vector<S*>()>;
    mUpcastVTable[key] = std::unique_ptr<JAny>(new JAnyT<upcast_fn_t>(std::move(upcast_lambda)));
}





