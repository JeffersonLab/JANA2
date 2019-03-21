
#include <greenfield/Scheduler.h>

namespace greenfield {


    FixedScheduler::FixedScheduler(Topology& topology, std::map<std::string, int> nthreads) :
            _topology(topology) {

        for (auto& pair : nthreads) {
            auto arrow_name = pair.first;
            auto arrow = topology.arrows[arrow_name]; // TODO: Safety
            auto threads = pair.second;
            for (int i = 0; i < threads; ++i) {
                _assignments.push_back(arrow);
            }
        }
    }


    bool FixedScheduler::rebalance(std::string from, std::string to, int delta) {

        Arrow* from_arrow = _topology.arrows[from];
        Arrow* to_arrow = _topology.arrows[to];
        size_t nworkers = _assignments.size();

        for (uint32_t i=0; i < nworkers && delta > 0; ++i) {
            if (_assignments[i] == from_arrow) {
                _assignments[i] = to_arrow;
            }
        }
        return delta == 0;
    };


    Arrow* FixedScheduler::next_assignment(const Report &report) {
        // TODO: Do something with report
        uint32_t worker_id = report.worker_id;
        if (worker_id < _assignments.size()) {
            return _assignments[report.worker_id];
        } else {
            return nullptr;
        }
    }


}