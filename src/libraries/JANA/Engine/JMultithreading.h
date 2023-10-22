

#pragma once
#include <mutex>
#include "JArrow.h"
#include "JArrowTopology.h"

class JMultithreading {


    std::mutex m_mutex;


    struct ArrowState {
        enum class Status { Unopened, Running, Paused, Finished };

        JArrow* arrow_ptr;
        Status status = Status::Unopened;
        bool is_parallel;
        int running_upstreams = 0;
        int running_instances = 0;
        std::vector<JArrow*> listeners; 
    }

    std::vector<ArrowState> arrow_states;
    int backoff_tries;
    int running_arrows;

    std::shared_ptr<JArrowTopology> m_topology;
    size_t m_next_idx;



public:
    // Previously on JArrow
    // Called by Arrow subclasses
    void run_arrow(JArrow* arrow);
    void pause_arrow(JArrow* arrow);
    void finish_arrow(JArrow* arrow);

    // 

    

};


std::ostream& operator<<(std::ostream& os, const JMultithreading::Status& s) 
