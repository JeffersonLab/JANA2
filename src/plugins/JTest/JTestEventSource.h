#pragma once

#include <vector>
#include <memory>
#include <random>

#include "JEventSource.h"
#include "JEvent.h"
#include "JQueueSimple.h"
#include "JApplication.h"
#include "JResourcePool.h"
#include "JSourceFactoryGenerator.h"
#include "JEventSourceGeneratorT.h"
#include "JQueueWithLock.h"
#include "JQueueWithBarriers.h"

#include "JTestDataObject.h"
#include "JPerfUtils.h"

class JTestEventSource : public JEventSource {
public:

    JTestEventSource(string source_name, JApplication *japp) : JEventSource(source_name, japp)
    {
        mNumEventsToGenerate = 5000;
        japp->GetJParameterManager()->SetDefaultParameter(
                "NEVENTS",
                mNumEventsToGenerate,
                "Number of events for fake event source to generate");

        //Make factory generator that will make factories for all types provided by the event source
        //This is necessary because the JFactorySet needs all factories ahead of time
        //Make sure that all types are listed as template arguments here!!
        mFactoryGenerator = new JSourceFactoryGenerator<>();

        auto params = mApplication->GetJParameterManager();
        mEventQueue = new JQueueSimple(params ,"Events", 200, 50);
    }

    ~JTestEventSource() {};

    static std::string GetDescription() {
        return "JTest Fake Event Source";
    }

    std::string GetType(void) const {
        return GetDemangledName<decltype(*this)>();
    }

    std::type_index GetDerivedType(void) const {
        return std::type_index(typeid(JTestEventSource));
    } //So that we only execute factory generator once per type

    void Open() {};

    void GetEvent(std::shared_ptr<JEvent> event) {

        if (mNumEventsToGenerate != 0 && mNumEventsToGenerate <= mNumEventsGenerated) {
            throw JEventSource::RETURN_STATUS::kNO_MORE_EVENTS;
        }
        mNumEventsGenerated++;
        event->SetEventNumber(mNumEventsGenerated);
        event->SetRunNumber(1234);
    }


private:

    std::size_t mNumEventsToGenerate;
    std::size_t mNumEventsGenerated = 0;

};


// This ensures sources supplied by other plugins that use the default CheckOpenable
// which returns 0.01 will still win out over this one.
template<> double JEventSourceGeneratorT<JTestEventSource>::CheckOpenable(std::string source) { return 1.0E-6; }



