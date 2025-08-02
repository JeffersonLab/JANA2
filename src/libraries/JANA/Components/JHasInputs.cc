
#include "JHasInputs.h"

#include <JANA/JEvent.h>
#include <JANA/Utils/JEventLevel.h>
#include <typeindex>

namespace jana::components {


JFactorySet* GetFactorySetAtLevel(const JEvent& event, JEventLevel desired_level) {

    // FIXME: event.GetParent() will except if parent is missing, disregarding 'is_optional'
    if (desired_level == JEventLevel::None || desired_level == event.GetLevel()) {
        return event.GetFactorySet();
    }
    return event.GetParent(desired_level).GetFactorySet();
}

JHasInputs::InputBase::~InputBase() {};

JHasInputs::VariadicInputBase::~VariadicInputBase() {};

void JHasInputs::InputBase::TriggerFactoryCreate(const JEvent& event) {
    auto facset = GetFactorySetAtLevel(event, m_level);
    if (facset == nullptr && !m_is_optional) {
        throw JException("Could not find parent at level=" + toString(m_level));
    }
    auto databundle = facset->GetDatabundle(m_type_index, m_databundle_name);
    if (databundle == nullptr && !m_is_optional) {
        facset->Print();
        throw JException("Could not find databundle with type_index=" + m_type_name + " and tag=" + m_databundle_name);
    }
    if (databundle != nullptr) {
        auto fac = databundle->GetFactory();
        if (fac != nullptr) {
            fac->Create(event);
        }
    }
}

void JHasInputs::VariadicInputBase::TriggerFactoryCreate(const JEvent& event) {
    auto facset = GetFactorySetAtLevel(event, m_level);
    if (facset == nullptr && !m_is_optional) {
        throw JException("Could not find parent at level=" + toString(m_level));
    }
    if (!m_realized_databundle_names.empty()) {
        for (auto& tag : m_requested_databundle_names) {
            auto coll = facset->GetDatabundle(m_type_index, tag);
            if (coll == nullptr && !m_is_optional) {
                facset->Print();
                throw JException("Could not find databundle with type_index=" + m_type_name + " and tag=" + tag);
            }
            if (coll != nullptr) {
                auto fac = coll->GetFactory();
                if (fac != nullptr) {
                    fac->Create(event);
                }
            }
        }
    }
    else if (m_empty_input_policy == EmptyInputPolicy::IncludeEverything) {
        auto databundles = facset->GetDatabundles(m_type_index);
        for (auto* databundle : databundles) {
            auto* factory = databundle->GetFactory();
            if (factory != nullptr) {
                factory->Create(event);
            }
        }
    }
}

} // namespace jana::components
