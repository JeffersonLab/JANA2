
#include <catch.hpp>
#include <JANA/JObject.h>

struct ContainerBase {
    size_t item_count;
};

template <typename T>
struct BasicDataContainer : ContainerBase {
    std::vector<T*> contents;

    void Set(std::vector<T*>& source) {
        for (T* item : source) {
            contents.push_back(item);
        }
    }
    void Set(std::vector<T*>&& source) {
        for (T* item : source) {
            contents.push_back(item);
        }
    }
    void Set(T* source) { 
        contents.push_back(source);
    }
    void Get(std::vector<T*>& destination) {
        for (T* item : contents) {
            destination.push_back(item);
        }
    }
    void Get(std::vector<void*>& destination) {
        for (T* item : contents) {
            destination.push_back(item);
        }
    }
    void Get(T& destination) {
        if (contents.size() != 1) {
            throw std::runtime_error("Wrong contents size!");
        }
        destination = *contents[0];
    }
};


struct JObjectDataContainerBase : ContainerBase {
    virtual void Set(std::vector<JObject*>& source) = 0;
    virtual void Get(std::vector<JObject*>& destination) = 0;
};

template <typename T>
struct JObjectDataContainer : public JObjectDataContainerBase {
    std::vector<T*> contents;
    
    void Set(std::vector<JObject*>& source) {
        for (JObject* item : source) {
            T* t = dynamic_cast<T*>(item);
            if (t == nullptr) {
                throw std::runtime_error("Bad cast!");
            }
            contents.push_back(t);
        }
    }

    void Get(std::vector<JObject*>& destination) {
        for (T* item : contents) {
            destination.push_back(item);
        }
    }

    void Set(std::vector<T*>& source) {
        for (T* item : source) {
            contents.push_back(item);
        }
    }
    void Set(std::vector<T*>&& source) {
        for (T* item : source) {
            contents.push_back(item);
        }
    }
    void Set(T* source) { 
        contents.push_back(source);
    }
    void Get(std::vector<T*>& destination) {
        for (T* item : contents) {
            destination.push_back(item);
        }
    }
    void Get(std::vector<void*>& destination) {
        for (T* item : contents) {
            destination.push_back(item);
        }
    }
    void Get(T& destination) {
        if (contents.size() != 1) {
            throw std::runtime_error("Wrong contents size!");
        }
        destination = *contents[0];
    }
};


template <typename T>
struct PodioDataContainer : ContainerBase {
    void* collection;
    void* frame;

    void Put(void* collection) {};
    void Get(void** collection) {};
    // TODO: Problem: Accessing the podio frame
};

template <typename EventT>
struct FactoryBase {
    bool has_data = false;
    virtual void Process(EventT& event) = 0;
    virtual size_t Create(EventT& event) = 0;
    virtual ContainerBase* GetData() = 0;
};

template <typename EventT, template <typename> typename ContainerT=BasicDataContainer>
struct Factory : public FactoryBase<EventT> {

    ContainerT<int> data;

    ContainerBase* GetData() override {
        return &data;
    }

    void Process(EventT& event) override {
        int* x = new int;
        *x = 22;
        data.Set(x);
    }

    size_t Create(EventT& event) {
        Process(event);
        this->has_data = true;
        return data.item_count;
    }

    template <typename DestT>
    void GetOrCreate(EventT& event, DestT& dest) {
        if (!this->has_data) {
            Create(event);
        }
        data.Get(dest);
    }
};

struct WEvent {
};


TEST_CASE("Basic") {
    WEvent event;
    Factory<WEvent> factory;

    std::vector<int*> results;
    factory.GetOrCreate(event, results);

    REQUIRE(results.size() == 1);
    REQUIRE(*results[0] == 22);
}

