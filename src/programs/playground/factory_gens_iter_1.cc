#include <map>
#include <iostream>
#include <JANA/JFactoryT.h>
#include <JANA/JObject.h>

struct SimpleFactoryGenerator {
    virtual JFactory* create() = 0;
};

template <typename T>
struct SimpleFactoryGeneratorT : SimpleFactoryGenerator {
    JFactory* create() override {
        return new T();
    }
};

using JFactoryGeneratorMap = std::map<std::type_index, SimpleFactoryGenerator*>;
using JFactoryMap = std::map<std::pair<std::type_index, std::string>, JFactory*>;

class Demonstration {

    JFactoryGeneratorMap m_factory_generators;
    JFactoryMap m_factories;

public:

    template <typename F, typename T>
    void add() {
        m_factory_generators[typeid(T)] = new SimpleFactoryGeneratorT<F>();
    }

    template<typename T>
    JFactoryT<T>* get(std::string tag) {
        auto key = std::make_pair(std::type_index(typeid(T)), tag);
        auto it = m_factories.find(key);
        if (it != m_factories.end()) {
            return static_cast<JFactoryT<T>*>(it->second);
        }
        auto gen = m_factory_generators.at(std::type_index(typeid(T)));
        auto fac = static_cast<JFactoryT<T>*>(gen->create());
        m_factories[key] = fac;
        return fac;
    }
};


struct DummyData : public JObject {
    double x = 22.2;
};

struct DummyFactory : public JFactoryT<DummyData> {
    double y = 44;
};


int main_old() {

    Demonstration demo;
    demo.add<DummyFactory, DummyData>();
    auto fac = demo.get<DummyData>(""); // returns JFactoryT<DummyData>, not DummyFactory
    // This is (supposedly) a feature: we don't _want_ to know which Factory we used!
    // If we did know exactly which factory was used, we would be breaking the decoupling!
    // Instead, we could be creating Factories through template specialization instead of inheritance
    fac->ClearData();
    return 0;

}





