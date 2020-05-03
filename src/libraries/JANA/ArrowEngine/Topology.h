
#ifndef JANA2_TOPOLOGY_H
#define JANA2_TOPOLOGY_H

#include <JANA/Services/JServiceLocator.h>
#include <JANA/ArrowEngine/Arrow.h>
#include <JANA/Utils/JEventPool.h>
#include <JANA/Engine/JEventSourceArrow.h>
#include <JANA/Utils/JProcessorMapping.h>

namespace jana {
namespace arrowengine {

struct Topology : public JService {
    virtual const std::vector<Arrow*> &get_arrows() = 0;
    virtual void initialize() = 0;
    virtual void open() = 0;
    virtual void close() = 0;
};

/// GenericTopology knows nothing about the JANA component interfaces. It is mainly used for
/// testing and improving the ArrowEngine internals, but it could come in handy if someone wants
/// to use the ArrowEngine on its own.
class GenericTopology : public Topology {
    std::vector<Arrow*> arrows;
    std::vector<Arrow*> sources;
    std::vector<Arrow*> sinks;
public:
    const std::vector<Arrow*> &get_arrows() override { return arrows; }
    void add_arrow(Arrow* arrow) { arrows.push_back(arrow); }  // Topology _doesn't_ take ownership of Arrow
    void initialize() override {};
    void open() override {};
    void close() override {};
};

/// DefaultJanaTopology assumes all of its arrows come from the JComponentManager. This is
/// sufficient for 'standard' use cases, but is insufficient if we need blocks or subevents.
class DefaultJanaTopology : public Topology {

    std::shared_ptr<JComponentManager> components;
    std::shared_ptr<JParameterManager> params;

    int granularity = 2;
    int event_pool_size = 1;
    std::vector<Arrow*> arrows;
    std::shared_ptr<JEventPool> event_pool; // TODO: Belongs somewhere else
    JProcessorMapping mapping; // TODO: Make this be a service as well?
    JLogger logger;

public:
    const std::vector<Arrow*> &get_arrows() override { return arrows; }
    void initialize() override;
    void open() override;
    void close() override;
};

} // namespace arrowengine
} // namespace jana

#endif //JANA2_TOPOLOGY_H
