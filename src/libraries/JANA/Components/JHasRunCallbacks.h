// Copyright 2024, Jefferson Science Associates, LLC.
// Subject to the terms in the LICENSE file found in the top-level directory.
// Created by Nathan Brei

#pragma once

#include <JANA/JEvent.h>

namespace jana::components {


class JHasRunCallbacks {

protected:
    struct ResourceBase {
        virtual void ChangeRun(int32_t run_nr, JApplication* app) = 0;
    };

    std::vector<ResourceBase*> m_resources;
    int32_t m_last_run_number = -1;

public:
    void RegisterResource(ResourceBase* resource) {
        m_resources.push_back(resource);
    }

    template <typename ServiceT, typename ResourceT, typename LambdaT>
    class Resource : public ResourceBase {
        ResourceT m_data;
        LambdaT m_lambda;

    public:

        Resource(JHasRunCallbacks* owner, LambdaT lambda) : m_lambda(lambda) {
            owner->RegisterResource(this);
        };

        const ResourceT& operator()() { return m_data; }

    protected:

        void ChangeRun(int32_t run_nr, JApplication* app) override {
            std::shared_ptr<ServiceT> service = app->template GetService<ServiceT>();
            m_data = m_lambda(service, run_nr);
        }
    };

    // Declarative interface
    virtual void ChangeRun(int32_t /*run_nr*/) {}

    // Full interface
    virtual void ChangeRun(const JEvent&) {}

    // Compatibility interface
    virtual void BeginRun(const std::shared_ptr<const JEvent>&) {}

    virtual void ChangeRun(const std::shared_ptr<const JEvent>&) {}

    virtual void EndRun() {}

};


} // namespace jana::components
