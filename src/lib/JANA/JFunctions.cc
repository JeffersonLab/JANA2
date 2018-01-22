#include "JFunctions.h"
#include "JEvent.h"
#include "JTask.h"
#include "JApplication.h"

std::shared_ptr<JTaskBase> JMakeAnalyzeEventTask(std::shared_ptr<JEvent>& aEvent, JApplication* aApplication)
{
	std::vector<JEventProcessor*> sProcessors;
	aApplication->GetJEventProcessors(sProcessors);
	auto sRunProcessors = [sProcessors](std::shared_ptr<JEvent>& aEvent) -> void
	{
		for(auto sProcessor : sProcessors)
			sProcessor->Process(aEvent);
	};

	//TODO: Get task from pool!
	auto sPackagedTask = std::packaged_task<void(std::shared_ptr<JEvent>&)>(sRunProcessors);
	auto sTask = std::make_shared<JTask<void>>(aEvent, sPackagedTask);
	return std::static_pointer_cast<JTaskBase>(sTask);
}
