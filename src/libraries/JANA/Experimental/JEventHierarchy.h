#pragma once

#include <JANA/Experimental/JOmniFactorySet.h>

/// Default event hierarchy

struct JRun : public HasKey<size_t>, 
              public HasFactories<JRun> {

    JRun() = default;
    JRun(size_t run_nr) {
        this->key = run_nr;
    }
    size_t GetRunNr() { return this->key.value(); }
};

struct JBlock : public HasParent<JRun>, 
                public HasKey<size_t>, 
                public HasFactories<JBlock> {

    JBlock() = default;
    JBlock(JRun* parent, size_t block_nr) {
        this->parent = parent;
        this->key = block_nr;
    }
    size_t GetBlockNr() { return this->key.value(); }
    JRun& GetRun() {return *parent;}
};

struct JPhysicsEvent : public HasParent<JBlock>, 
                       public HasKey<size_t>,
                       public HasFactories<JPhysicsEvent> {


    JPhysicsEvent() = default;
    JPhysicsEvent(JBlock* block, size_t event_nr) {
        this->parent = block;
        this->key = event_nr;
    }
    size_t GetEventNr() {return this->key.value();}
    JBlock& GetBlock() {return *parent;}
};

struct JSubevent : public HasParent<JPhysicsEvent>, 
                   public HasKey<size_t>,
                   public HasFactories<JSubevent> {

    JSubevent() = default;
    JSubevent(JPhysicsEvent* parent, size_t subevent_nr) {
        this->parent = parent;
        this->key = subevent_nr;
    }
    size_t GetSubeventNr() {return this->key.value();}
    JPhysicsEvent& GetEvent() {return *parent;}
};



