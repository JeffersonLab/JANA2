#ifndef _jana_test_factory_
#define _jana_test_factory_

#include <JANA/JFactoryT.h>
#include <JANA/JEvent.h>
#include "JTestDummyData.h"
#include "JTestSourceData1.h"
#include "JTestSourceData2.h"

#include <random>
#include <chrono>
#include <iostream>

class JTestDummyDataFactory : public JFactoryT<JTestDummyData> {

    std::mt19937 mRandomGenerator;

public:
    JTestDummyDataFactory() : JFactoryT<JTestDummyData>("jana_test_factory") {
        // Seed random number generator // TODO: Move this to JTestHelpers
        auto sTime = std::chrono::high_resolution_clock::now().time_since_epoch().count();
        mRandomGenerator.seed(sTime);
    }

    ~JTestDummyDataFactory() {};

    void Init() {
        jout << "jana_test_factory::Init() called " << std::endl;
    }

    void ChangeRun(const std::shared_ptr<const JEvent> &aEvent) {

        auto prior_run = GetPreviousRunNumber();
        auto current_run = aEvent->GetRunNumber();

        std::cout << "JTestDummyDataFactory::ChangeRun(): " << prior_run
                  << "=>" << current_run << std::endl;
    }

    void Process(const std::shared_ptr<const JEvent> &aEvent) {
        // This factory will grab the JSourceObject and JSourceObject2 types created by
        // the source. For each JSourceObject2 objects, it will create a jana_test
        // object (product of this factory). For each of those, it submits a JTask which
        // implements some busy work to represent calculations done to produce the jana_test
        // objects.

        // Get the JSourceObject and JSourceObject2 objects
        auto sobjs = aEvent->GetT<JTestSourceData1>();
        auto sobj2s = aEvent->GetT<JTestSourceData2>();

        // Create a jana_test object for each JSourceObject2 object
        std::vector<std::shared_ptr<JTaskBase> > sTasks;
        sTasks.reserve(sobj2s.size());
        for (auto sobj2 : sobj2s) {
            auto jtest = new JTestDummyData();
            mData.push_back(jtest);

            // Make a lambda that does busy work representing what would be done to
            // to calculate the jana_test objects from the inout objects.
            //
            // n.b. The JTask mechanism is hardwired to pass only the JEvent reference
            // as an argument. All other parameters must be passed as capture variables.
            // (e.g. the jtest and sObj2 objects)
            auto sMyLambda = [=](const std::shared_ptr<const JEvent> &aEvent) -> double {

                // Busy work
                std::mt19937 sRandomGenerator; // need dedicated generator to avoid thread conflict
                auto sTime = std::chrono::high_resolution_clock::now().time_since_epoch().count();
                sRandomGenerator.seed(sTime);
                double c = sobj2->GetHitE();
                for (int i = 0; i < 1000; i++) {
                    double a = sRandomGenerator();
                    double b = sqrt(a * pow(1.23, -a)) / a;
                    c += b;
                    jtest->AddRandom(a); // add to jana_test object
                }
                jtest->SetE(c); // set energy of jana_test object

                return c / 10.0; // more complicated than one normally needs but demonstrates use of a return value
            };

            // Make task shared_ptr (Return type of lambda is double)
            auto sTask = std::make_shared<JTask<double> >();

            // Set the event and the lambda
            sTask->SetEvent(aEvent);
            sTask->SetTask(std::packaged_task<double(const std::shared_ptr<const JEvent> &)>(sMyLambda));

            // Move the task onto the vector (to avoid changing shptr ref count). casts at the same time
            sTasks.push_back(std::move(sTask));
        }

        // Submit the tasks to the queues in the thread manager.
        // This function won't return until all of the tasks are finished.
        // This thread will help execute tasks (hopefully these) in the meantime.
        (*aEvent).GetJApplication()->GetJThreadManager()->SubmitTasks(sTasks);

        // For purposes of example, we grab the return values from the tasks
        double sSum = 0.0;
        for (auto sTask : sTasks) {
            auto &st = static_cast< JTask<double> & >(*sTask);
            sSum += st.GetResult();
        }
    }

};

#endif // _jana_test_factory_

