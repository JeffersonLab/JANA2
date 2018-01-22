#ifndef _JTask_h_
#define _JTask_h_

#include <future>
#include <memory>

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
		JTaskBase(std::shared_ptr<JEvent>& aEvent) : mEvent(aEvent) { }
		virtual ~JTaskBase(void) = default;

		void SetEvent(std::shared_ptr<JEvent>& aEvent){mEvent = aEvent;}
		JEvent* GetEvent(void) {return mEvent.get();}

		virtual bool IsFinished(void) const = 0;

		//EXECUTE
		virtual void operator()(void) = 0;

	protected:
		std::shared_ptr<JEvent> mEvent;
};

template <typename ReturnType>
class JTask : public JTaskBase
{
	public:

		//STRUCTORS
		JTask(void) = delete;
		JTask(std::shared_ptr<JEvent>& aEvent, std::packaged_task<ReturnType(std::shared_ptr<JEvent>&)>&& aTask) : JTaskBase(aEvent), mTask(std::move(aTask)) { }

		void Set_Task(std::packaged_task<ReturnType(std::shared_ptr<JEvent>&)>&& aTask){mTask = std::move(aTask); mFuture = mTask.get_future();}

		bool IsFinished(void) const{return (mFuture.wait_for(std::chrono::seconds(0)) == std::future_status::ready);}
		ReturnType GetResult(void){return mFuture.get();}

		//EXECUTE
		void operator()(void){mTask(mEvent);}

	private:
		std::packaged_task<ReturnType(std::shared_ptr<JEvent>&)> mTask;
		std::future<ReturnType> mFuture;
};

#endif // _JTask_h_
