
#pragma once
#include <JANA/Omni/JComponentFwd.h>


class JServiceLocator;
struct JService : public jana::omni::JComponent {

    /// acquire_services is a callback which allows the user to configure a JService
    /// which relies on other JServices. The idea is that the user uses a constructor
    /// or initialize() method to configure things which don't rely on other JServices, and
    /// then use acquire_services() to configure the things which do. We need this because
    /// due to JANA's plugin architecture, we can't guarantee the order in which JServices
    /// get provided. So instead, we collect all of the JServices first and wire them
    /// together afterwards in a separate phase.
    ///
    /// Note: Don't call JApplication::GetService() or JServiceLocator::get() from InitPlugin()!

    virtual ~JService() = default;

    // This will be deprecated eventually
    virtual void acquire_services(JServiceLocator*) {};
};

