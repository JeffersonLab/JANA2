
#ifndef JANA2_TOPOLOGY_H
#define JANA2_TOPOLOGY_H

namespace jana {
namespace arrowengine {

struct Topology : public JService {
    virtual const std::vector<JArrow *> &get_arrows() = 0;
    virtual const JArrowStatus &get_status() = 0;
    virtual void initialize() = 0;
    virtual void open() = 0;
    virtual void close() = 0;
}

/// GenericTopology knows nothing about the JANA component interfaces. It is mainly used for
/// testing and improving the ArrowEngine internals, but it could come in handy if someone wants
/// to use the ArrowEngine on its own.
class GenericTopology : public Topology {
    std::vector<JArrow*> arrows;
    std::vector<JArrow*> sources;
    std::vector<JArrow*> sinks;
public:
    const std::vector<JArrow *> &get_arrows() override;
    const JArrowStatus &get_status() override;
    void initialize() override;
    void open() override;
    void close() override;
};

/// DefaultJanaTopology assumes all of its arrows come from the JComponentManager. This is
/// sufficient for 'standard' use cases, but is insufficient if we need blocks or subevents.
class DefaultJanaTopology : public Topology {
    std::shared_ptr<JEventPool> event_pool; // TODO: Belongs somewhere else
    Status status = Inactive; // TODO: Merge this concept with JActivable

    std::vector<JArrow*> arrows;
    std::vector<JArrow*> sources;           // Sources needed for activation
    std::vector<JArrow*> sinks;             // Sinks needed for finished message count // TODO: Not anymore
    std::vector<EventQueue*> queues;        // Queues shared between arrows
    JProcessorMapping mapping;

    JLogger _logger;
public:
    const std::vector<JArrow *> &get_arrows() override;
    const JArrowStatus &get_status() override;
    void initialize() override;
    void open() override;
    void close() override;
};


} // namespace arrowengine
} // namespace jana

#endif //JANA2_TOPOLOGY_H
