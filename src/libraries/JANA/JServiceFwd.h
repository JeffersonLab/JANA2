
#pragma once
#include <JANA/Components/JComponentFwd.h>


class JServiceLocator;
struct JService : public jana::components::JComponent {

    /// JServices may require other JServices, which may be loaded out of order from
    /// different plugins. For this reason, initialization of JServices is delayed until
    /// all plugins are loaded. Suppose you are writing Service B which depends on Service A. 
    /// To acquire A during B's initialization, you should
    /// either declare a `Service<A> m_a_svc{this}` helper member, or fetch it manually by calling 
    /// `GetApplication()->GetService<A>()` from inside `B::Init()`. Either way, the JServices will 
    /// be initialized recursively and in the correct order.
    ///
    /// Note that historically JServices used the `acquire_services()` callback instead of `Init()`.
    /// While `acquire_services` hasn't been deprecated yet, it is on the roadmap, so we strongly
    /// recommend you override `Init()` instead.
    ///
    /// Note: Don't call JApplication::GetService<SvcT>() or JServiceLocator::get<SvcT>() from InitPlugin()!

    virtual ~JService() = default;

    // JService has its own DoInit() for the sake of acquire_services()
    void DoInit(JServiceLocator*);

    // This will be deprecated eventually
    virtual void acquire_services(JServiceLocator*) {};
};

