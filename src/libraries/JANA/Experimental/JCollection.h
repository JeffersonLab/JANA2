#pragma once
#include <typeindex>

struct JCollection {
    std::type_index type_index; // TODO: Might not need this anymore
    std::string type_name;
    std::string collection_name;

    bool is_owning = true;
    bool is_persistent = false;
    bool is_prefixed = false;

    JCollection(std::type_index type_index, std::string type_name, std::string collection_name) :
        type_index(type_index),
        type_name(type_name),
        collection_name(collection_name) {}

    virtual ~JCollection() = default;
    virtual size_t GetSize() = 0;
    virtual void Reset() = 0;
};

template <typename T>
struct JPodioCollection : public JCollection {
    using OuterType = T*;
    using InnerType = T;

    OuterType collection;

    OuterType& operator()() {
        return collection;
    }

    InnerType& operator()(size_t index) {
        return collection[index];
    }

    size_t GetSize() override {
        return 0;
    }

    void Reset() override {
    }
};

template <typename T>
struct JBasicCollection : public JCollection {

    using OuterType = std::vector<const T*>;
    using InnerType = const T;

    OuterType contents;

    OuterType& operator()() {
        return contents;
    }

    InnerType& operator()(size_t index) {
        return *(contents[index]);
    }

    size_t GetSize() override {
        return contents.size();
    }

    void Reset() override {
        if (is_owning) {
            for (const T* item : contents) {
                delete item;
            }
        }
        contents.clear();
    }

    JBasicCollection(std::string name) : JCollection(std::type_index(typeid(T)), JTypeInfo::demangle<T>(), name) {
    }
};


