
#include <greenfield/Scheduler.h>

namespace greenfield {


    FixedScheduler::FixedScheduler(Topology& topology, std::map<std::string, int> nthreads) :
            _topology(topology) {

        for (auto& pair : nthreads) {
            auto arrow_name = pair.first;
            auto arrow = topology.get_arrow(arrow_name);
            auto threads = pair.second;
            for (int i = 0; i < threads; ++i) {
                _assignments.push_back(arrow);
            }
        }
    }


    bool FixedScheduler::rebalance(std::string from, std::string to, int delta) {

        Arrow* from_arrow = _topology.get_arrow(from);
        Arrow* to_arrow = _topology.get_arrow(to);
        size_t nworkers = _assignments.size();

        for (uint32_t i=0; i < nworkers && delta > 0; ++i) {
            if (_assignments[i] == from_arrow) {
                _assignments[i] = to_arrow;
                delta--;
            }
        }
        return delta == 0;
    };


    Arrow* FixedScheduler::next_assignment(uint32_t worker_id, Arrow* assignment, StreamStatus last_result) {

        if (worker_id >= _assignments.size()) {
            return nullptr;
        }
        else if (last_result == StreamStatus::Finished) {
            LOG_DEBUG(logger) << "FixedScheduler: Worker " << worker_id << " finished => Idling." << LOG_END;
            return nullptr;
        }
        LOG_DEBUG(logger) << "FixedScheduler: Worker " << worker_id << " => "
                          << _assignments[worker_id]->get_name() << LOG_END;

        return _assignments[worker_id];
    }

}