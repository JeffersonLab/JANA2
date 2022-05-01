
// Copyright 2020, Jefferson Science Associates, LLC.
// Subject to the terms in the LICENSE file found in the top-level directory.

#include <vector>

#include <JANA/JFactorySet.h>

#ifndef _JFactoryGenerator_h_
#define _JFactoryGenerator_h_

class JFactoryGenerator {

    std::string m_plugin_name;

public:

    virtual ~JFactoryGenerator() = default;

    virtual void GenerateFactories(JFactorySet *factory_set) = 0;

    inline std::string GetPluginName() {
        return m_plugin_name;
    }

    inline void SetPluginName(std::string plugin_name) {
        m_plugin_name = std::move(plugin_name);
    }
};


template<class T>
class JFactoryGeneratorT : public JFactoryGenerator {

    std::string m_tag;
    bool m_tag_specified;

public:

    JFactoryGeneratorT() : m_tag_specified(false) {}
    explicit JFactoryGeneratorT(std::string tag) : m_tag(std::move(tag)), m_tag_specified(true) {};

    void GenerateFactories(JFactorySet *factory_set) override {
        JFactory *factory = new T;

        if (m_tag_specified) {
            // If user specified a tag via the generator (even the empty tag!), use that.
            // Otherwise, use whatever tag the factory may have set for itself.
            factory->SetTag(m_tag);
        }
        factory->SetFactoryName(JTypeInfo::demangle<T>());
        factory->SetPluginName(GetPluginName());
        factory_set->Add(factory);
    }
};


#endif // _JFactoryGenerator_h_


