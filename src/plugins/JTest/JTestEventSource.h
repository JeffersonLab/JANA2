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

        japp->GetJParameterManager()->SetDefaultParameter(
                "JTEST:INCLUDE_BARRIER_EVENTS",
                mIncludeBarriers,
                "Include barrier events");

        //Make factory generator that will make factories for all types provided by the event source
        //This is necessary because the JFactorySet needs all factories ahead of time
        //Make sure that all types are listed as template arguments here!!
        mFactoryGenerator = new JSourceFactoryGenerator<JTestSourceData1, JTestSourceData2>();

        auto params = mApplication->GetJParameterManager();

        // Event queue: max size of 200, keep at least 50 buffered
        if (mIncludeBarriers) {
            mEventQueue = new JQueueWithBarriers(params, "Events", 200, 50);
        }
        else {
            mEventQueue = new JQueueSimple(params ,"Events", 200, 50);
        }
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

        if (mIncludeBarriers) {
            auto sIsBarrierEvent = (mNumEventsGenerated % mBarrierFrequency) == 0;
            event->SetIsBarrierEvent(sIsBarrierEvent);
        }
        event->SetEventNumber(mNumEventsGenerated);
        event->SetRunNumber(1234);
    }


    bool GetObjects(const std::shared_ptr<const JEvent> &aEvent,
                                      JFactory *aFactory) {

        //Get objects of the specified type, and set them in the factory
        //If this type is not supplied by the file, return false. Else return true.

        //For all of the objects retrievable from the file, we MUST generate factories via our own JFactoryGenerator
        auto sTypeIndex = aFactory->GetObjectType();
        if (sTypeIndex == std::type_index(typeid(JTestSourceData1)))
            return GetObjects(aEvent, static_cast<JFactoryT<JTestSourceData1> *>(aFactory));
        else if (sTypeIndex == std::type_index(typeid(JTestSourceData2)))
            return GetObjects(aEvent, static_cast<JFactoryT<JTestSourceData2> *>(aFactory));
        else
            return false;
    }


    bool GetObjects(const std::shared_ptr<const JEvent> &aEvent,
                    JFactoryT<JTestSourceData1> *aFactory) {

        if (aFactory->GetTag() != "") return false; //Only default tag here

        size_t sNumObjects = randint(1,20);
        std::vector<JTestSourceData1 *> sObjects;
        for (size_t si = 0; si < sNumObjects; si++) {

            // Create new JSourceObject
            auto sObject = new JTestSourceData1(randdouble(), si);
            sObjects.push_back(sObject);

            // Pretend to be parsing input data
            writeMemory(sObject->mRandoms, randint(1000,2000));
        }

        //Set the objects in the factory
        aFactory->Set(std::move(sObjects));
        return true;
    }


    bool GetObjects(const std::shared_ptr<const JEvent> &aEvent,
                    JFactoryT<JTestSourceData2> *aFactory) {

        if (aFactory->GetTag() != "") return false; //Only default tag here

        size_t sNumObjects = randint(1,20);
        std::vector<JTestSourceData2 *> sObjects;
        for (size_t si = 0; si < sNumObjects; si++) {

            // Create new JSourceObject
            auto sObject = new JTestSourceData2(randdouble(), si);
            sObjects.push_back(sObject);

            // Pretend to be parsing input data
            writeMemory(sObject->mRandoms, randint(1000,2000));
        }

        //Set the objects in the factory
        aFactory->Set(std::move(sObjects));
        return true;
    }


private:

    std::size_t mNumEventsToGenerate;
    std::size_t mNumEventsGenerated = 0;
    std::size_t mBarrierFrequency = 100;

    bool mIncludeBarriers = false;
};


// This ensures sources supplied by other plugins that use the default CheckOpenable
// which returns 0.01 will still win out over this one.
template<> double JEventSourceGeneratorT<JTestEventSource>::CheckOpenable(std::string source) { return 1.0E-6; }



