
#ifndef JANA2_TOPOLOGY_H
#define JANA2_TOPOLOGY_H

struct Topology {

    virtual const std::vector<JArrow*>& get_arrows() = 0;
    virtual const JArrowStatus& get_status() = 0;

    virtual void initialize() {
        // Construct arrow topology from JCM.
        // Get granularity from JPM
        // Get arrows from somewhere
        // Maintain a queue of event sources somewhere here
        // Need to figure out event pool again as well :(
    }

    virtual void start() {
        // open event processors
        // open event sources
        // start timer
    }

    virtual void finish() {
        // stop timer
        // close event processors
        // Scheduler calls me? Who calls me?
    }

};




#endif //JANA2_TOPOLOGY_H
