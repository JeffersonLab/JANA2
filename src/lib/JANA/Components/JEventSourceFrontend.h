//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// Jefferson Science Associates LLC Copyright Notice:
//
// Copyright 251 2014 Jefferson Science Associates LLC All Rights Reserved. Redistribution
// and use in source and binary forms, with or without modification, are permitted as a
// licensed user provided that the following conditions are met:
// 1. Redistributions of source code must retain the above copyright notice, this
//    list of conditions and the following disclaimer.
// 2. Redistributions in binary form must reproduce the above copyright notice, this
//    list of conditions and the following disclaimer in the documentation and/or other
//    materials provided with the distribution.
// 3. The name of the author may not be used to endorse or promote products derived
//    from this software without specific prior written permission.
// This material resulted from work developed under a United States Government Contract.
// The Government retains a paid-up, nonexclusive, irrevocable worldwide license in such
// copyrighted data to reproduce, distribute copies to the public, prepare derivative works,
// perform publicly and display publicly and to permit others to do so.
// THIS SOFTWARE IS PROVIDED BY JEFFERSON SCIENCE ASSOCIATES LLC "AS IS" AND ANY EXPRESS
// OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
// MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL
// JEFFERSON SCIENCE ASSOCIATES, LLC OR THE U.S. GOVERNMENT BE LIABLE TO LICENSEE OR ANY
// THIRD PARTES FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
// DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
// OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
// LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
// OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
// POSSIBILITY OF SUCH DAMAGE.
//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
//
// Author: Nathan Brei
//

#ifndef JANA2_JEVENTSOURCEFRONTEND_H
#define JANA2_JEVENTSOURCEFRONTEND_H

#include <JANA/JException.h>
#include <JANA/Utils/JTypeInfo.h>
#include <JANA/Components/V1/jerror.h>
#include <set>
#include <memory>


class JFactoryGenerator;
class JEvent;
class JApplication;
class JFactory;

namespace jana {
namespace v1 {
class JEventSource {

public:

    JEventSource(const char *source_name);

    virtual ~JEventSource();

    virtual const char *className(void) { return static_className(); }

    static const char *static_className(void) { return "JEventSource"; }

    // TODO: Probably need a JEventV1 wrapper
    virtual jerror_t GetEvent(JEvent &event) = 0;   ///< Get the next event from this source
    virtual void FreeEvent(JEvent &event);      ///< Free an event that is no longer needed.
    virtual jerror_t GetObjects(JEvent &event, JFactory *factory); ///< Get objects from an event

    inline const char *GetSourceName(void) { return source_name.c_str(); } ///< Get this sources name
    bool IsFinished(void);

protected:
    std::string source_name;
    int source_is_open;
    pthread_mutex_t read_mutex;
    pthread_mutex_t in_progress_mutex;
    int Nevents_read;
    bool done_reading; // set to true after all events have been read (determined by return value from call to GetEvent() )
    std::set <uint64_t> in_progess_events;
    uint64_t Ncalls_to_GetEvent;

    inline void LockRead(void) { pthread_mutex_lock(&read_mutex); }

    inline void UnlockRead(void) { pthread_mutex_unlock(&read_mutex); }
};

}

namespace v2 {

class JEventSource {

    const std::string m_resource_name;
    JApplication *m_application;
    std::string m_type_name;
    JFactoryGenerator *m_factory_generator;

public:
    enum class RETURN_STATUS { kSUCCESS, kNO_MORE_EVENTS, kBUSY, kTRY_AGAIN, kERROR, kUNKNOWN };


    explicit JEventSource(std::string resource_name, JApplication *app = nullptr)
            : m_resource_name(std::move(resource_name)), m_application(app) {}

    virtual ~JEventSource() {}

    void SetTypeName(std::string type_name) { m_type_name = std::move(type_name); }
    std::string GetTypeName() { return m_type_name; }

    void SetFactoryGenerator(JFactoryGenerator *generator) { m_factory_generator = generator; }
    JFactoryGenerator* GetFactoryGenerator() { return m_factory_generator; }

    std::string GetName() const { return m_resource_name; }


    // Methods which user should implement

    virtual void Open() {}

    virtual void GetEvent(std::shared_ptr <JEvent>) = 0;

    virtual bool GetObjects(const std::shared_ptr<const JEvent> &, JFactory *) { return false; }

};
}


namespace v3 {

class JEventSource {
    std::string m_type_name;

public:
    enum class Result {
        SUCCESS, FAILURE_BUSY, FAILURE_FINISHED
    };

    // data
    void set_type_name(std::string type_name) { m_type_name = std::move(type_name); }
    std::string get_type_name() { return m_type_name; }

    // methods
    virtual void open(std::string resource_name) {}
    virtual void close() {}
    virtual Result next_event(JEvent &) = 0;
};
} // namespace v3
} // namespace jana

#endif //JANA2_JEVENTSOURCEFRONTEND_H
