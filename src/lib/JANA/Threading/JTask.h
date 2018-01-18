#ifndef _JTask_h_
#define _JTask_h_

#include <future>

namespace JANA { namespace Threading
{

//These classes are used to erase the return-type of the packaged_task (and package the system and priority)

/**************************************************************** TYPE DECLARATIONS ****************************************************************/

class JTaskBase;

template <typename ReturnType>
class JTask;

/**************************************************************** TYPE DEFINITION ****************************************************************/

class JTaskBase
{
	public:
		//STRUCTORS
		virtual ~JTaskBase(void) = default;

		//EXECUTE
		virtual void operator()(void) = 0;
};

template <typename ReturnType>
class JTask : public JTaskBase
{
	public:

		//STRUCTORS
		JTask(void) = delete;
		JTask(std::packaged_task<ReturnType(void)>&& aTask) : JTaskBase(), mTask(std::move(aTask)) { }

		void Set_Task(std::packaged_task<ReturnType(void)>&& aTask){mTask = std::move(aTask);}
		std::future<ReturnType> Get_Future(void){return mTask.get_future();} //Can only be called once!

		//EXECUTE
		void operator()(void){mTask();}

	private:
		std::packaged_task<ReturnType(void)> mTask;
};

}} //end namespaces

#endif // _JTask_h_
