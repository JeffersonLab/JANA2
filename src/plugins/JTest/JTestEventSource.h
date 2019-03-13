// $Id$
//
//    File: JTestEventSource.h
// Created: Mon Oct 23 22:39:45 EDT 2017
// Creator: davidl (on Darwin harriet 15.6.0 i386)
//

#ifndef _JEventSource_jana_test_
#define _JEventSource_jana_test_

#include <vector>
#include <memory>
#include <random>

#include <JANA/JEventSource.h>
#include <JANA/JEvent.h>
#include <JANA/JQueueSimple.h>
#include <JANA/JApplication.h>
#include "JResourcePool.h"
#include "JSourceFactoryGenerator.h"
#include "JEventSourceGeneratorT.h"
#include "JQueueWithLock.h"
#include "JQueueWithBarriers.h"

#include "JTestSourceData1.h"
#include "JTestSourceData2.h"

thread_local std::mt19937 gRandomGenerator;

class JTestEventSource : public JEventSource {
public:

    JTestEventSource(string source_name, JApplication *japp) : JEventSource(source_name, japp)
    {
        mNumEventsToGenerate = 20000;
        japp->GetJParameterManager()->SetDefaultParameter(
                "NEVENTS",
                mNumEventsToGenerate,
                "Number of events for fake event source to generate");

        japp->GetJParameterManager()->SetDefaultParameter(
                "JTEST:INCLUDE_BARRIER_EVENTS",
                mIncludeBarriers,
                "Include barrier events");

        //Seed random number generator //not ideal!
        auto sTime = std::chrono::high_resolution_clock::now().time_since_epoch().count();
        mRandomGenerator.seed(sTime);

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

    std::shared_ptr<const JEvent> GetEvent() {

        if (mNumEventsToGenerate != 0 && mNumEventsToGenerate <= mNumEventsGenerated) {
            throw JEventSource::RETURN_STATUS::kNO_MORE_EVENTS;
        }
        mNumEventsGenerated++;

        //These are recycled, so be sure to re-set EVERY member variable
        auto sEvent = mEventPool.Get_SharedResource(mApplication);

        if (mIncludeBarriers) {
            auto sIsBarrierEvent = (mNumEventsGenerated % mBarrierFrequency) == 0;
            sEvent->SetIsBarrierEvent(sIsBarrierEvent);
        }
        sEvent->SetEventNumber(mNumEventsGenerated);
        sEvent->SetRunNumber(1234);
        return sEvent;
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

        // Generate a random # of objects
        auto sNumObjectsDistribution = std::uniform_int_distribution<std::size_t>(1, 20);
        auto sNumObjects = sNumObjectsDistribution(gRandomGenerator);
        std::vector<JTestSourceData1 *> sObjects;
        for (std::size_t si = 0; si < sNumObjects; si++) {
            // Create new JSourceObject
            auto sObject = new JTestSourceData1(gRandomGenerator(), si);
            sObjects.push_back(sObject);

            // Supply busy work to take time. This would represent
            // the parsing of the input data.
            auto sNumRandomsDistribution = std::uniform_int_distribution<std::size_t>(1000, 2000);
            auto sNumRandoms = sNumRandomsDistribution(gRandomGenerator);
            for (std::size_t sj = 0; sj < sNumRandoms; sj++) sObject->AddRandom(gRandomGenerator());
        }

        //Set the objects in the factory
        aFactory->Set(std::move(sObjects));
        return true;
    }


    bool GetObjects(const std::shared_ptr<const JEvent> &aEvent,
                    JFactoryT<JTestSourceData2> *aFactory) {

        if (aFactory->GetTag() != "") return false; //Only default tag here

        // Generate a random # of objects
        auto sNumObjectsDistribution = std::uniform_int_distribution<std::size_t>(1, 20);
        auto sNumObjects = sNumObjectsDistribution(gRandomGenerator);
        std::vector<JTestSourceData2 *> sObjects;
        double sSum = 0.0;
        for (std::size_t si = 0; si < sNumObjects; si++) {
            // Create new JSourceObject
            auto sObject = new JTestSourceData2(gRandomGenerator(), si);
            sObjects.push_back(sObject);

#if 0
            // Supply busy work to take time. This would represent
        // the parsing of the input data.
        auto sNumRandomsDistribution = std::uniform_int_distribution<std::size_t>(10, 200);
        auto sNumRandoms = sNumRandomsDistribution(gRandomGenerator);
        for(std::size_t sj = 0; sj < sNumRandoms; sj++) {

            double c = 0.0;
            for(int i=0; i<1000; i++){
                double a = gRandomGenerator();
                double b = sqrt( a*pow(1.23, -a))/a;
                c += b;
            }
            sObject->AddRandom(c);
            sSum += c;
        }
#endif
            sObject->SetHitE(sSum);
        }

        //Set the objects in the factory
        aFactory->Set(std::move(sObjects));
        return true;
    }



private:

    std::size_t mNumEventsToGenerate;
    std::size_t mNumEventsGenerated = 0;
    std::size_t mBarrierFrequency = 100;
    std::mt19937 mRandomGenerator;


    bool mIncludeBarriers = true;
    JResourcePool<JEvent> mEventPool;
};


// This ensures sources supplied by other plugins that use the default CheckOpenable
// which returns 0.01 will still win out over this one.
template<> double JEventSourceGeneratorT<JTestEventSource>::CheckOpenable(std::string source) { return 1.0E-6; }


#endif // _JEventSourceGenerator_jana_test_

