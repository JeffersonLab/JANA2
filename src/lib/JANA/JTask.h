#ifndef _JTask_h_
#define _JTask_h_

#include <future>
#include <memory>
#include <chrono>

//These classes are used to erase the return-type of the packaged_task (and package the system and priority)

/**************************************************************** TYPE DECLARATIONS ****************************************************************/

class JTaskBase;
class JEvent;

template <typename ReturnType>
class JTask;

/**************************************************************** TYPE DEFINITION ****************************************************************/

class JTaskBase
{
	public:
		//STRUCTORS
		JTaskBase(std::shared_ptr<JEvent*>& aEvent) : mEvent(aEvent) { }
		virtual ~JTaskBase(void) = default;

		void SetEvent(std::shared_ptr<JEvent*>& aEvent){mEvent = aEvent;}
		JEvent* GetEvent(void) {return mEvent.get();}

		virtual bool IsFinished(void) const = 0;

		//EXECUTE
		virtual void operator()(void) = 0;

	protected:
		std::shared_ptr<JEvent*> mEvent;
};

template <typename ReturnType>
class JTask : public JTaskBase
{
	public:

		//STRUCTORS
		JTask(void) = delete;
		JTask(std::shared_ptr<JEvent*>& aEvent, std::packaged_task<ReturnType(std::shared_ptr<JEvent*>&)>&& aTask) : JTaskBase(aEvent), mTask(std::move(aTask)) { }

		void Set_Task(std::packaged_task<ReturnType(std::shared_ptr<JEvent*>&)>&& aTask){mTask = std::move(aTask); mFuture = mTask.get_future();}

		bool IsFinished(void) const{return (mFuture.wait_for(std::chrono::seconds(0)) == std::future_status::ready);}
		ReturnType GetResult(void){return mFuture.get();}

		//EXECUTE
		void operator()(void){mTask(mEvent);}

	private:
		std::packaged_task<ReturnType(std::shared_ptr<JEvent*>&)> mTask;
		std::future<ReturnType> mFuture;
};

void SubmitAnalyzeEventTask(std::shared_ptr<JEvent*>& aEvent, JApplication* aApplication)
{
	auto sProcessors = aApplication->GetJEventProcessors();
	auto sRunProcessors = [sProcessors](std::shared_ptr<JEvent*>& aEvent) -> void
	{
		for(auto sProcessor : sProcessors)
			sProcessor->Process(aEvent);
	};

	//TODO: Get task from pool!
	auto sPackagedTask = std::packaged_task<void(std::shared_ptr<JEvent*>&)>(sRunProcessors);
	auto sTask = std::make_shared<JTask<void>>(aEvent, sPackagedTask);
	aApplication->GetJThreadManager()->Submit_AsyncTasks({sTask}, JQueueSet::JQueueType::Events);

}

std::shared_ptr<JTask<void>> MakeGetEventTask(JApplication* aApplication, JEventSource* aEventSource)
{
	auto sGetEvent = [aEventSource, aApplication](void) -> void
	{
		std::pair<>
		while(true)
		{
			auto sEventPair = aEventSource->GetEvent();
			if(sEventPair.first == JEventSource::kTRY_AGAIN)
				continue;
			else if(sEventPair.first == JEventSource::kSUCCESS)
				continue;
		}
		kSUCCESS,
		kNO_MORE_EVENTS,
		,
		kUNKNOWN

		auto sAnalyzeEventTask = SubmitAnalyzeEventTask(sEventPair.second, aApplication);
	};

	//TODO: Get task from pool!
	auto sPackagedTask = std::packaged_task<void(std::shared_ptr<JEvent*>&)>(sGetEvent);
	auto sTask = std::make_shared<JTask<void>>(nullptr, sPackagedTask);
}

#endif // _JTask_h_
