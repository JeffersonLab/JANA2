
#pragma once

#include <JANA/Utils/JTypeInfo.h>
#include <JANA/Experimental/JCollection.h>
#include <JANA/JObject.h>
#include <mutex>


template <typename ParentT>
struct HasParent {
    using ParentType = ParentT;
    ParentT* parent = nullptr;
    size_t sibling_count;
    size_t sibling_id;
};

template <typename KeyT>
struct HasKey {
    std::optional<KeyT> key;
    using KeyType = KeyT;
};

template <typename LevelT>
struct HasFactories;


template <typename> struct InputBase;

template <typename LevelT>
struct HasInputs {
    std::vector<InputBase<LevelT>*> inputs;

    void RegisterInput(InputBase<LevelT>* input) {
        inputs.push_back(input);
    }
};

template <typename LevelT>
struct InputBase {
    std::vector<std::string> collection_names;
    virtual void GetData(LevelT& event) = 0;
};

template <typename LevelT, typename CollT>
struct Input : public InputBase<LevelT> {
    CollT* m_collection;

    Input(HasInputs<LevelT>* owner, std::string collection_name="") {
        owner->RegisterInput(this);
        this->collection_names.push_back(collection_name);
    }

    const typename CollT::OuterType& operator()() { 
        if (m_collection == nullptr) {
            throw JException("Missing collection: Was GetData() called?");
        }
        return (*m_collection)();
    }

    void GetData(LevelT& event) override {
        m_collection = event.template GetOrCreate<CollT>(this->collection_names[0]);
    }
};

template <typename LevelT, typename CollT>
struct OptionalInput : public InputBase<LevelT> {
    CollT* m_collection;

    OptionalInput(HasInputs<LevelT>* owner, std::string collection_name="") {
        owner->RegisterInput(this);
        this->collection_names.push_back(collection_name);
    }

    std::optional<const typename CollT::OuterType*> operator()() { 
        if (m_collection == nullptr) {
            return std::nullopt;
        }
        return &(m_collection->contents);
    }

    void GetData(LevelT& event) override {
        m_collection = event.template GetOrCreate<CollT>(this->collection_names[0]);
    }
};

template <typename LevelT, typename CollT>
struct VariadicInput : public InputBase<LevelT> {
    std::vector<CollT*> m_collections;

    VariadicInput(HasInputs<LevelT>* owner, std::vector<std::string> collection_names) {
        owner->RegisterInput(this);
        this->collection_names = collection_names;
    }

    const std::vector<const typename CollT::OuterType*> operator()() { 
        std::vector<const typename CollT::OuterType*> collections;
        for (auto* coll : m_collections) {
            if (coll == nullptr) {
                throw JException("Missing collection: Was GetData() called?");
            }
            collections.push_back(&((*coll)()));
        }
        return collections;
    }

    void GetData(LevelT& event) override {
        for (const auto& c : this->collection_names) {
            m_collections.push_back(event.template GetOrCreate<CollT>(c));
        }
    }
};




struct OutputBase {
    virtual JCollection* GetCollection() = 0;
};

struct HasOutputs {
    std::vector<OutputBase*> outputs;

    void RegisterOutput(OutputBase* output) {
        outputs.push_back(output);
    }
};

template <typename CollT>
struct Output : public OutputBase {
    CollT collection;

    Output(HasOutputs* owner, std::string default_collection_name="") 
    : collection(default_collection_name) {
        owner->RegisterOutput(this);
    }

    typename CollT::OuterType& operator()() { return collection(); }

    JCollection* GetCollection() override { return &collection;}
};



template <typename T, typename=void>
struct HasChangeFn : public HasInputs<T> {
    std::optional<typename T::KeyType> prev_key;

    bool DoChange(T& t) {
        if ((prev_key == std::nullopt) || (prev_key != t.key)) {
            prev_key = t.key;
            for (InputBase<T>* input : HasInputs<T>::inputs) {
                input->GetData(t);
            }
            Change(t);
            return true;
        }
        return false;
    }
    virtual void Change(T&) {}
};

template <typename T>
struct HasChangeFn<T, std::void_t<decltype(T::parent)>> : public HasChangeFn<typename T::ParentType>, public HasInputs<T> {
    std::optional<typename T::KeyType> prev_key;
    using HasChangeFn<typename T::ParentType>::Change; // suppress Clang's "hidden overloaded virtual function" warning

    bool DoChange(T& t) {
        bool parent_changed = (t.parent != nullptr) && (HasChangeFn<typename T::ParentType>::DoChange(*t.parent));
        if (parent_changed || (prev_key == std::nullopt) || (prev_key != t.key)) {
            prev_key = t.key;
            for (InputBase<T>* input : HasInputs<T>::inputs) {
                input->GetData(t);
            }
            Change(t);
            return true;
        }
        return false;
    }

    virtual void Change(T&) {}
};

template <typename T, typename=void>
struct HasProcessFn : public HasInputs<T> {
    bool process_ran = false;
    std::mutex mutex;

    void DoProcess(T& t) {
        std::lock_guard<std::mutex> lock(mutex);
        if (!process_ran) {
            for (InputBase<T>* input : HasInputs<T>::inputs) {
                input->GetData(t);
            }
            Process(t);
            process_ran = true;
        }
    }
    virtual void Process(T&) {}
};

template <typename T>
struct HasProcessFn<T, std::void_t<decltype(T::parent)>> 
    : public HasChangeFn<typename T::ParentType>,
      public HasInputs<T> {

    bool process_ran = false;
    std::mutex mutex;

    void DoProcess(T& t) {
        std::lock_guard<std::mutex> lock(mutex);
        if (!process_ran) {
            if (t.parent != nullptr) {
                HasChangeFn<typename T::ParentType>::DoChange(*t.parent);
            }
            for (InputBase<T>* input : HasInputs<T>::inputs) {
                input->GetData(t);
            }
            Process(t);
            // TODO: Finalize output data
            process_ran = true;
        }
    }
    virtual void Process(T&) {}
};

template <typename LevelT> 
struct JOmniFactory : public HasProcessFn<LevelT>, 
                      public HasOutputs {

        virtual ~JOmniFactory() = default;
};

template <typename LevelT> 
struct JOmniProcessor : public HasProcessFn<LevelT> {
        virtual ~JOmniProcessor() = default;
};


