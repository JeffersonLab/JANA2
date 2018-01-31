#ifndef _JTask_h_
#define _JTask_h_

#include <future>
#include <memory>

#include "JResettable.h"
//JTaskBase is used to erase the return-type of the packaged_task
//JTask must be used in a shared_ptr
//This is because std::future and std::packaged_task are non-copyable (only movable)
//And often two pieces of code want to use the JTask (the JQueue and the code waiting for the result)
//We could use a normal pointer, but the thread executing the JTask doesn't know if anyone is waiting for the result or not.
//Thus we must use shared_ptr to ensure that it is delted (or recycled) when it goes out of scope.

/**************************************************************** TYPE DECLARATIONS ****************************************************************/

class JTaskBase;
class JEvent;

template <typename ReturnType>
class JTask;

/**************************************************************** TYPE DEFINITIONS ****************************************************************/

class JTaskBase : public JResettable
{
	public:

		//STRUCTORS
		JTaskBase(void) = default;
		virtual ~JTaskBase(void) = default;

		//MOVERS
		JTaskBase(JTaskBase&&) = default;
		JTaskBase& operator=(JTaskBase&&) = default;

		void SetEvent(std::shared_ptr<JEvent>& aEvent){mEvent = aEvent;}
		void SetEvent(std::shared_ptr<JEvent>&& aEvent){mEvent = std::move(aEvent);}
		JEvent* GetEvent(void) {return mEvent.get();} //don't increase ref count

		virtual bool IsFinished(void) const = 0;

		//OPERATORS
		virtual void operator()(void) = 0;

		//RESOURCES
		void Release(void){mEvent = nullptr;} 	//Release all (pointers to) resources, called when recycled to pool
		void Reset(void){}; 					//Re-initialize the object, called when retrieved from pool

	protected:
		std::shared_ptr<JEvent> mEvent;
};

template <typename ReturnType>
class JTask : public JTaskBase
{
	public:
		//STRUCTORS
		JTask(void) = default;

		//MOVERS
		JTask(JTask&&) = default;
		JTask& operator=(JTask&&) = default;

		void SetTask(std::packaged_task<ReturnType(std::shared_ptr<JEvent>&)>&& aTask){mTask = std::move(aTask); mFuture = mTask.get_future();}

		bool IsFinished(void) const{return (mFuture.wait_for(std::chrono::seconds(0)) == std::future_status::ready);}
		ReturnType GetResult(void){return mFuture.get();}

		//EXECUTE
		void operator()(void){mTask(mEvent);}

	private:
		std::packaged_task<ReturnType(std::shared_ptr<JEvent>&)> mTask;
		std::future<ReturnType> mFuture;
};

#endif // _JTask_h_
