
#include <catch.hpp>
#include <JANA/Utils/JTypeInfo.h>

namespace jana {
namespace hier {


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

template <typename> struct InputBase;
// template <typename, typename> struct Input;
// TODO: using Input;

template <typename LevelT>
struct HasInputs {
    std::vector<InputBase<LevelT>*> inputs;

    void RegisterInput(InputBase<LevelT>* input) {
        inputs.push_back(input);
    }
};

template <typename LevelT>
struct InputBase {
    std::string type_name;
    std::string collection_name;
    virtual void GetData(LevelT& t) = 0;
};

template <typename LevelT, typename DataT>
struct Input {
    std::vector<const LevelT*> m_data;

    Input(HasInputs<LevelT>* owner, std::string default_tag="") {
        owner->RegisterInput(this);
        this->collection_name = default_tag;
        this->type_name = JTypeInfo::demangle<DataT>();
    }
    const std::vector<const DataT*>& operator()() { return m_data; }

    void GetData(LevelT& level) {
        // TODO: What is retrieval from these things going to look like? Reimagine JFactorySet
        // m_data = event.Get<T>(this->collection_name);
    }
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
struct HasChangeFn<T, std::void_t<decltype(T::parent)>> : public HasChangeFn<typename T::ParentType> {
    std::optional<typename T::KeyType> prev_key;
    using HasChangeFn<typename T::ParentType>::Change; // suppress Clang's "hidden overloaded virtual function" warning

    bool DoChange(T& t) {
        bool parent_changed = HasChangeFn<typename T::ParentType>::DoChange(*t.parent);
        if (parent_changed || (prev_key == std::nullopt) || (prev_key != t.key)) {
            prev_key = t.key;
            Change(t);
            return true;
        }
        return false;
    }

    virtual void Change(T&) {}
};

template <typename T, typename=void>
struct HasProcessFn {
    void DoProcess(T& t) {
        Process(t);
    }
    virtual void Process(T&) {}
};

template <typename T>
struct HasProcessFn<T, std::void_t<decltype(T::parent)>> : public HasChangeFn<typename T::ParentType>{
    void DoProcess(T& t) {
        HasChangeFn<typename T::ParentType>::DoChange(*t.parent);
        Process(t);
    }
    virtual void Process(T&) {}
};

template <typename T>
struct Factory;


struct Block : HasKey<size_t> {
    Factory<Block>* fac;
    size_t GetBlockNr() { return this->key.value(); }
};


struct Event : public HasParent<Block>, public HasKey<size_t> {
    Factory<Event>* fac;
    size_t GetEventNr() {return this->key.value();}
    Block& GetBlock() {return *parent;}
};

struct Subevent : public HasParent<Event>, public HasKey<size_t> {
    Factory<Subevent>* fac;
    size_t GetSubeventNr() {return this->key.value();}
    Event& GetEvent() {return *parent;}
};


template <typename T> 
struct Factory : public HasProcessFn<T> {

    int data = 0;

    int get(T& t) {
        HasProcessFn<T>::DoProcess(t);
        return data;
    }
};



struct TestBFac : public Factory<Block> {

    bool changeBlock_called = false;

    void Process(Block&) override {
        changeBlock_called = true;
        this->data = 17;
    }
};

struct TestEFac : public Factory<Event> {

    bool changeBlock_called = false;
    bool changeEvent_called = false;

    void Change(Block&) override {
        changeBlock_called = true;
    }
    void Process(Event&) override {
        changeEvent_called = true;
        data = 99;
    }
};


struct TestSFac : public Factory<Subevent> {

    bool changeBlock_called = false;
    bool changeEvent_called = false;
    bool process_called = false;

    void Change(Block&) override {
        REQUIRE(changeBlock_called == false);
        REQUIRE(changeEvent_called == false);
        REQUIRE(process_called == false);
        changeBlock_called = true;
    }
    void Change(Event&) override {
        REQUIRE(changeBlock_called == true);
        REQUIRE(changeEvent_called == false);
        REQUIRE(process_called == false);
        changeEvent_called = true;
    }
    void Process(Subevent&) override {
        REQUIRE(changeBlock_called == true);
        REQUIRE(changeEvent_called == true);
        REQUIRE(process_called == false);
        process_called = true;
        this->data = 77;
    }
};



TEST_CASE("Experiment") {

    Block block;
    Event event;
    Subevent subevent;

    event.parent = &block;
    subevent.parent = &event;

    block.key = 22;
    event.key = 13;
    subevent.key = 99;

    TestBFac f;
    int bdata = f.get(block);
    REQUIRE(bdata == 17);

    TestEFac ff;
    int fdata = ff.get(event);
    REQUIRE(fdata == 99);
    REQUIRE(ff.changeBlock_called == true);
    REQUIRE(ff.changeEvent_called == true);

    TestSFac fff;
    int sdata = fff.get(subevent);
    REQUIRE(sdata == 77);
    REQUIRE(fff.changeBlock_called == true);
    REQUIRE(fff.changeEvent_called == true);
    REQUIRE(fff.process_called == true);


}

}}
