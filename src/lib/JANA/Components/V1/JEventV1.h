// $Id: JEvent.h 1042 2005-06-14 20:48:00Z davidl $
//
//    File: JEvent.h
// Created: Wed Jun  8 12:30:53 EDT 2005
// Creator: davidl (on Darwin wire129.jlab.org 7.8.0 powerpc)
//

/// The JANA1 concept of an event is basically a whole bunch of bookkeeping information.

#ifndef _JEvent_V1_
#define _JEvent_V1_

#include <JANA/JEvent.h>
#include <JANA/JApplication.h>

#include <iostream>
#include <vector>
#include <string>



// Place everything in JANA namespace
namespace jana {
namespace v1 {

// For compatibility with V2
using JFactory_base = JFactory;
#define jout std::cout

class JEventLoop;

class JEvent {
private:
    // Things that we wrap

    // Extraneous things

    uint64_t event_TS = 0;
    void* ref = nullptr;
    JEventLoop* loop = nullptr
    uint64_t status = 0L;
    uint64_t id = 0;
    bool sequential = false;  ///< set in event source to treat this as a barrier event (i.e. no other events will be processed in parallel with this one)

public:
    JEvent(JEvent& event, JApplication* app) : m_event(event), m_app(app) {};

    virtual ~JEvent() {};

    virtual const char* className() { return static_className(); }

    static const char* static_className() { return "JEvent"; }


    template<class T>
    jerror_t GetObjects(vector<const T*>& t, JFactory_base* factory = nullptr);

    inline void FreeEvent(void) {
        throw JException("JEventSource::FreeEvent is no longer supported!");
        //if (source)source->JEventSource::FreeEvent(*this);
    }


    inline JEventSource* GetJEventSource() { return source; }

    inline int32_t GetRunNumber() { return run_number; }

    inline uint64_t GetEventNumber() { return event_number; }

    inline uint64_t GetEventTS() { return event_TS; }

    inline void* GetRef() { return ref; }

    inline JEventLoop* GetJEventLoop() { return loop; }

    inline bool GetSequential() { return sequential; }

    inline uint64_t GetID(void) const { return id; }


    inline void SetJEventSource(JEventSource* source) { this->source = source; }

    inline void SetRunNumber(int32_t run_number) { this->run_number = run_number; }

    inline void SetEventNumber(uint64_t event_number) { this->event_number = event_number; }

    inline void SetEventTS(uint64_t event_TS) { this->event_TS = event_TS; }

    inline void SetRef(void* ref) { this->ref = ref; }

    inline void SetJEventLoop(JEventLoop* loop) { this->loop = loop; }

    inline void SetSequential(bool s = true) { sequential = s; }

    inline void SetID(uint64_t id) { this->id = id; }



    inline void Print(void) {
        jout<<"JEvent: this=0x"<<hex<<(unsigned long)this<<dec;
        jout<<" source=0x"<<hex<<(unsigned long)source<<dec;
        jout<<" event_number="<<event_number;
        jout<<" run_number="<<run_number;
        jout<<" ref=0x"<<hex<<(unsigned long)ref<<dec;
        jout<<" loop=0x"<<hex<<(unsigned long)loop<<dec;
        jout<<" status=0x"<<hex<<status<<dec;
        jout<<" sequential="<<sequential;
        jout<<endl;
    };

    uint64_t GetStatus(void) { return status; }

    inline bool GetStatusBit(uint32_t bit) {
        /// Return the present value of the specified status bit.
        /// The value of "bit" should be from 0-63.
        return (status>>bit) & 0x01;
    };

    void SetStatus(uint64_t status) { this->status = status; }

    bool SetStatusBit(uint32_t bit, bool val = true) {

        /// Set the value of the specified status bit. If the
        /// second argument is passed, the bit will be set to
        /// that value. Otherwise, the bit will be set to "true".
        /// The value of "bit" should be from 0-63.
        /// The value of the status bit prior to  this call is
        /// returned.

        bool old_val = (status>>bit) & 0x01;

        uint64_t mask = ((uint64_t)0x01)<<bit;

        if(val){
            // Set bit
            status |= mask;
        }else{
            // Clear bit
            status &= ~mask;
        }

        return old_val;
    }

    bool ClearStatusBit(uint32_t bit) {
        /// Clear the specified status bit.
        /// The value of "bit" should be from 0-63.
        /// This is equivalent to calling SetStatusBit(bit, false).
        /// The value of the status bit prior to  this call is
        /// returned.

        bool old_val = (status>>bit) & 0x01;
        uint64_t mask = ((uint64_t)0x01)<<bit;
        status &= ~mask;
        return old_val;
    }

    inline void ClearStatus(void) {
        /// Clear all bits in the status word. This
        /// is equivalent to calling SetStatus(0).
        status = 0L;
    };

    /* Commented out in the hope that nobody is actually using this

    void SetStatusBitDescription(uint32_t bit, string description) {
        if(japp)japp->SetStatusBitDescription(bit, description);
    };

    string GetStatusBitDescription(uint32_t bit) {
        if(japp) return japp->GetStatusBitDescription(bit);
        return "no description available";
    };

    void GetStatusBitDescriptions(map <uint32_t, string>& status_bit_descriptions) {
        if(japp)japp->GetStatusBitDescriptions(status_bit_descriptions);
    };
    */

};


//---------------------------------
// GetObjects
//---------------------------------
template<class T>
jerror_t JEvent::GetObjects(vector<const T*>& t, JFactory_base* factory) {
    /// Call the GetObjects() method of the associated source.
    /// This will cause the objects of the appropriate type and tag
    /// to be read in from the source and stored in the factory (if they
    /// exist in the source).

    // Make sure source is at least not NULL
    if (!source)throw JException(string("JEvent::GetObjects called when source is NULL"));

    // Get list of object pointers. This will read the objects in
    // from the source, instantiating them and handing ownership of
    // them over to the factory object.
    jerror_t err = source->GetObjects(*this, factory);
    if (err != NOERROR)return err; // if OBJECT_NOT_AVAILABLE is returned, the source could not provide the objects

    // OK, must have found some objects (possibly even zero) in the source.
    // Copy the pointers into the passed reference. To do this we
    // need to cast the JFactory_base pointer into a JFactory pointer
    // of the proper flavor. This also adds a tiny layer of protection
    // in case the factory pointer passed doesn't correspond to a
    // JFactory based on type T.
    JFactory <T>* fac = dynamic_cast<JFactory <T>*>(factory);
    if (fac) {
        fac->CopyFrom(t);
    } else {
        // If we get here, it means "factory" is not based on "T".
        // Actually, it turns out a long standing bug in g++ can
        // cause dynamic_cast to fail for objects created in a routine
        // attached via libdl. Because of this, we have to check the
        // name of the data class. I am leaving the above dynamic_cast
        // here because it *should* be the proper way to do this and
        // does actually work most of the time. Hopefully, this will
        // be resolved at some point and we can remove this message
        // and use this code to flag true bugs.
        if (!strcmp(factory->GetDataClassName(), T::static_className())) {
            fac = (JFactory <T>*) factory;
            fac->CopyFrom(t);
        } else {
            std::cout << __FILE__ << ":" << __LINE__ << " BUG DETECTED!! email davidl@jlab.org and complain!"
                      << std::endl;
            std::cout << "factory->GetDataClassName()=\"" << factory->GetDataClassName() << "\"" << std::endl;
            std::cout << "T::className()=\"" << T::static_className() << "\"" << std::endl;
        }
    }

    return NOERROR;
}

} // namespace v1
} // namespace jana


#endif // _JEvent_

