
// Copyright 2020, Jefferson Science Associates, LLC.
// Subject to the terms in the LICENSE file found in the top-level directory.

#pragma once

#include <vector>
#include <JANA/JFactorySet.h>

class JApplication;

class JFactoryGenerator {

    std::string m_plugin_name;
    JApplication* m_app;

public:

    virtual ~JFactoryGenerator() = default;

    virtual void GenerateFactories(JFactorySet *factory_set) = 0;

    inline std::string GetPluginName() {
        return m_plugin_name;
    }

    inline void SetPluginName(std::string plugin_name) {
        m_plugin_name = std::move(plugin_name);
    }

    inline void SetApplication(JApplication* app) {
        m_app = app;
    }

    inline JApplication* GetApplication() {
        return m_app;
    }
};

/// JFactoryGeneratorT works for both JFactories and JMultifactories
template<class T>
class JFactoryGeneratorT : public JFactoryGenerator {

    std::string m_tag;
    bool m_tag_specified;

public:

    JFactoryGeneratorT() : m_tag_specified(false) {}
    explicit JFactoryGeneratorT(std::string tag) : m_tag(std::move(tag)), m_tag_specified(true) {};

    void GenerateFactories(JFactorySet *factory_set) override {
        auto* factory = new T;

        if (m_tag_specified) {
            // If user specified a tag via the generator (even the empty tag!), use that.
            // Otherwise, use whatever tag the factory may have set for itself.
            factory->SetTag(m_tag);
        }
        factory->SetTypeName(JTypeInfo::demangle<T>());
        factory->SetPluginName(GetPluginName());
        factory->SetApplication(GetApplication());
        factory->SetLogger(GetApplication()->GetJParameterManager()->GetLogger(factory->GetPrefix()));
        factory_set->Add(factory);
    }
};



