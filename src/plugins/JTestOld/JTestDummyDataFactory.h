#ifndef _jana_test_factory_
#define _jana_test_factory_

#include <JANA/JFactoryT.h>
#include <JANA/JEvent.h>
#include <JANA/JPerfUtils.h>
#include "JTestDataObject.h"

#include <random>
#include <chrono>
#include <iostream>

class JTestDummyDataFactory : public JFactoryT<JTestDataObject> {

public:
    JTestDummyDataFactory() : JFactoryT<JTestDataObject>("JTestDummyDataFactory") {}

    ~JTestDummyDataFactory() {};

    void Init() {
        //jout << "JTestDummyDataFactory::Init() called " << std::endl;
    }

    void ChangeRun(const std::shared_ptr<const JEvent> &aEvent) {

        //auto prior_run = GetPreviousRunNumber();
        //auto current_run = aEvent->GetRunNumber();

        //std::cout << "JTestDummyDataFactory::ChangeRun(): " << prior_run
        //          << "=>" << current_run << std::endl;
    }

    void Process(const std::shared_ptr<const JEvent> &aEvent) {
        // This factory will grab the JSourceObject and JSourceObject2 types created by
        // the source. For each JSourceObject2 objects, it will create a jana_test
        // object (product of this factory). For each of those, it submits a JTask which
        // implements some busy work to represent calculations done to produce the jana_test
        // objects.

        // Get the JSourceObject and JSourceObject2 objects
        auto sobjs = aEvent->Get<JTestSourceData1>();
        auto sobj2s = aEvent->Get<JTestSourceData2>();

        // Create a jana_test object for each JSourceObject2 object
        std::vector<std::shared_ptr<JTaskBase> > sTasks;
        sTasks.reserve(sobj2s.size());
        for (auto sobj2 : sobj2s) {
            auto jtest = new JTestDataObject();
            mData.push_back(jtest);

            // Make a lambda that does busy work representing what would be done to
            // to calculate the jana_test objects from the inout objects.
            //
            // n.b. The JTask mechanism is hardwired to pass only the JEvent reference
            // as an argument. All other parameters must be passed as capture variables.
            // (e.g. the jtest and sObj2 objects)
            auto sMyLambda = [=](const std::shared_ptr<const JEvent> &aEvent) -> double {
                double c = doBusyWork(jtest->mRandoms, sobj2->mE);
                jtest->mE = c;
                return c / 10.0;
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
        //(*aEvent).GetJApplication()->GetJThreadManager()->SubmitTasks(sTasks);

        // For purposes of example, we grab the return values from the tasks
        double sSum = 0.0;
        for (auto sTask : sTasks) {
            auto &st = static_cast< JTask<double> & >(*sTask);
            st();  // Actually evaluate the subtask
            sSum += st.GetResult();
        }
    }

};

#endif // _jana_test_factory_

