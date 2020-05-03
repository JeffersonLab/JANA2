#ifndef JANA2_COMPONENTARROWS_H
#define JANA2_COMPONENTARROWS_H

#include <JANA/ArrowEngine/Arrow.h>
#include <JANA/JEventSource.h>
#include <JANA/Utils/JEventPool.h>

namespace jana {
namespace arrowengine {

using Event = std::shared_ptr<JEvent>;

class EventSourceOp : public SourceOp<Event>{
    JEventSource* m_source;
    std::shared_ptr<JEventPool> m_pool;  // TODO: For NUMA support, replace with another arrow
    JLogger m_logger;

public:
    EventSourceOp(JEventSource* source, std::shared_ptr<JEventPool> pool)
        : m_source(source), m_pool(std::move(pool)) {};

    std::pair<Status, std::shared_ptr<JEvent> > next() override;
};

class EventProcessorOp : public MapOp<Event,Event> {
    std::vector<JEventProcessor*> m_processors;
    std::shared_ptr<JEventPool> m_pool;
    JLogger m_logger;

public:
    EventProcessorOp(std::vector<JEventProcessor*> processors, std::shared_ptr<JEventPool> pool)
        : m_processors(std::move(processors)), m_pool(std::move(pool)) {};

    Event map(Event) override;

};

class BlockArrow {

};

class SubeventArrow {

};





} // namespace arrowengine
} // namespace jana


#endif //JANA2_COMPONENTARROWS_H
