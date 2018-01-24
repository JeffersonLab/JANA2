#ifndef JResourcePool_h
#define JResourcePool_h

#include <atomic>
#include <typeinfo>
#include <vector>
#include <mutex>
#include <iostream>
#include <type_traits>
#include <memory>
#include <algorithm>

#include "JResettable.h"

/****************************************************** OVERVIEW ******************************************************
 *
 * This class can be used to pool resources between pools for a given object type, even on different threads.
 * The way this works is that, in addition to a "local" resource pool for each instantiated JResourcePool object,
 * there is a private static resource pool, which is thus shared amongst all pools of that type.
 *
 * So, if an object is requested and the sal pool is empty, it will then try to retrieve one from the shared pool.
 * If none exist, it makes a new one.
 * Also, if you a recycle an object and the sal pool is "full," it will save it to the shared pool instead.
 * The reason there is both a "local" pool and a shared pool is to minimize the locking needed.
 * Control variables can be set to define exactly how this behaves.
 *
 * A pool counter keeps track of how many JResourcePool's there are for a given type.
 * Once it drops to zero, all of the remaining objects are deleted.
 *
 ***************************************** DEFINITION AND USE: NON-SHARED_PTR'S ***************************************
 *
 * If you do not intend to retrieve the resources from this pool as shared_ptr's, then just define a pool (in e.g. a factory) as:
 * JResourcePool<MyClass> dMyClassPool;
 *
 * You can then retrieve and recycle resources via the Get_Resource() and Recycle() functions.
 * Be sure to recycle the memory once you are done with it (e.g. beginning of factory evnt() call) or else it will leak.
 *
 ******************************************* DEFINITION AND USE: SHARED_PTR'S *****************************************
 *
 * You can retrieve shared_ptr's of the objects by calling the Get_SharedResource() method.
 * The advantage of using shared_ptr's is that they automatically keep track of when they are out of scope.
 * These shared_ptr's have been created with a DSharedPtrRecycler functor:
 * Once the shared_ptr goes out of scope, the contained resource is automatically recycled back to the pool.
 *
 * Note that there is a tricky situation that arises when a shared_ptr outlives the life of the JResourcePool.
 * This can happen if (e.g.) shared_ptr's are stored as members of a factory alongside a JResourcePool, and the pool is destroyed first.
 * Or if they are created by a thread_local pool, and the objects outlive the thread.
 *
 * To combat this, we have the DSharedPtrRecycler hold a weak_ptr to the JResourcePool (where the object gets recycled to).
 * That way, if the pool has been deleted, the weak_ptr will have expired and we can manually delete the object instead of trying to recycle it.
 * This only works if the pool itself has been created within a shared_ptr in the first place, so it is recommended that these pools be defined as:
 *
 * YOU MUST DO THIS!!
 * auto dMyClassPool = std::make_shared<JResourcePool<MyClass>>();
 *
 *************************************************** FACTORY OBJECTS **************************************************
 *
 * If you want the _data objects for the factory to be managed by a resource pool instead of JANA, in the factory init() call:
 * SetFactoryFlag(NOT_OBJECT_OWNER);
 * With this flag set, JANA will not delete the objects in _data, but it will clear them (_data.clear()) prior to the evnt() method.
 * So that means you must also put them in a separate factory vector so that you have a handle on them to recycle them at the beginning of the factory evnt().
 *
 *************************************************** CLASS COMPONENTS **************************************************
 *
 * Note that the components of the particle classes (DKinematicData, DChargedTrackHypothesis, DNeutralParticleHypothesis) contain shared_ptr's.
 * That's because the components (kinematics, timing info, etc.) are often identical between objects, and instead of duplicating the memory, it's cheaper to just shared
 * For example, the kinematics for the pre-kinfit DChargedTrackHypothesis are identical to those from the DTrackTimeBased.
 * Also, the tracking information for the post-kinfit DChargedTrackHypothesis is identical to the pre-kinfit information.
 *
 * The resource pools for these shared_ptr's are defined to be private thread_local within the classes themselves.
 * That way each thread has an instance of the pool, while still sharing a common pool underneath.
 *
 *********************************************** BEWARE: CLASS COMPONENTS **********************************************
 *
 * Problem: Creating a resource on one thread, and recycling it on another thread: race condition in the resource pool sAL pool.
 *
 * How does this happen?  You create a charged-hypo on one thread, which creates a DKinematicInfo object on that thread.
 * You then recycle the hypo, and another thread picks it up.  The other thread calls Hypo::Reset(), which clears the DKinematicInfo.
 * Since it's stored in a shared_ptr, it goes back to the thread it was created.
 *
 * Solution 1: Disable intra-thread sharing of objects that CONTAIN shared_ptrs: Everything that is or inherits from DKinematicData
 * Do this by setting the max size of the shared pool to zero for these types.
 *
 * Solution 2: Have those objects inherit from JResettable (below), and define the member functions.
 *
 **********************************************************************************************************************/

// Apple compiler does not currently support alignas. Make this an empty definition if it is not already defined.
#ifndef alignas
#define alignas(A)
#endif

template <typename DType> class JResourcePool : public std::enable_shared_from_this<JResourcePool<DType>>
{
	//TYPE TRAIT REQUIREMENTS
	//If these statements are false, this won't compile
	static_assert(!std::is_pointer<DType>::value, "The template type for JResourcePool must not be a pointer (the stored type IS a pointer though).");
	static_assert(!std::is_const<DType>::value, "The template type for JResourcePool must not be const.");
	static_assert(!std::is_volatile<DType>::value, "The template type for JResourcePool must not be volatile.");

	public:
		JResourcePool(void);
		JResourcePool(std::size_t sGetBatchSize, std::size_t sNumToAllocateAtOnce, std::size_t sMaxLocalPoolSize);
		JResourcePool(std::size_t sGetBatchSize, std::size_t sNumToAllocateAtOnce, std::size_t sMaxLocalPoolSize, std::size_t sMaxSharedPoolSize, std::size_t sDebugLevel);
		~JResourcePool(void);

		void Set_ControlParams(std::size_t sGetBatchSize, std::size_t sNumToAllocateAtOnce, std::size_t sMaxLocalPoolSize);
		void Set_ControlParams(std::size_t sGetBatchSize, std::size_t sNumToAllocateAtOnce, std::size_t sMaxLocalPoolSize, std::size_t sMaxSharedPoolSize, std::size_t sDebugLevel);
		DType* Get_Resource(void);
		std::shared_ptr<DType> Get_SharedResource(void);

		//RECYCLE CONST OBJECTS //these methods just const_cast and call the non-const versions
		void Recycle(const DType* sResource){Recycle(const_cast<DType*>(sResource));}
		void Recycle(std::vector<const DType*>& sResources); //move-clears the input vector

		//RECYCLE NON-CONST OBJECTS
		void Recycle(DType* sResource);
		void Recycle(std::vector<DType*>& sResources); //move-clears the input vector

		std::size_t Get_SharedPoolSize(void) const;
		std::size_t Get_PoolSize(void) const{return mResourcePool_Local.size();}
		std::size_t Get_NumObjectsAllThreads(void) const{return dObjectCounter;}

		static constexpr unsigned int Get_CacheLineSize(void)
		{
			/// Returns the cache line size for the processor of the target platform.
			/*! The cache line size is useful for creating a buffer to make sure that a variable accessed by multiple threads does not share the cache line it is on.
			    This is useful for variables that may be written-to by one of the threads, because the thread will acquire locked access to the entire cache line.
			    This blocks other threads from operating on the other data stored on the cache line. Note that it is also important to align the shared data as well.
			    See http://www.drdobbs.com/parallel/eliminate-false-sharing/217500206?pgno=4 for more details. */

			//cache line size is 64 for ifarm1402, gcc won't allow larger than 128
			//the cache line size is in /sys/devices/system/cpu/cpu0/cache/index0/coherency_line_size
			return 64; //units are in bytes
		}

	private:

		//Enable this version if type inherits from JResettable //void: is return type
		template <typename RType> typename std::enable_if<std::is_base_of<JResettable, RType>::value, void>::type Release_Resources(JResettable* sResource){sResource->Release();}
		template <typename RType> typename std::enable_if<std::is_base_of<JResettable, RType>::value, void>::type Reset(JResettable* sResource){sResource->Reset();}

		//Enable this version if type does NOT inherit from JResettable //void: is return type
		template <typename RType> typename std::enable_if<!std::is_base_of<JResettable, RType>::value, void>::type Release_Resources(RType* sResource){};
		template <typename RType> typename std::enable_if<!std::is_base_of<JResettable, RType>::value, void>::type Reset(RType* sResource){};

		//Assume that access to the shared pool won't happen very often: will mostly access the thread-local pool (this object)
		void Get_Resources_StaticPool(void);
		void Recycle_Resources_StaticPool(std::vector<DType*>& sResources);

		alignas(Get_CacheLineSize()) std::size_t dDebugLevel = 0;
		alignas(Get_CacheLineSize()) std::size_t dGetBatchSize = 100;
		alignas(Get_CacheLineSize()) std::size_t dNumToAllocateAtOnce = 20;
		alignas(Get_CacheLineSize()) std::size_t dMaxLocalPoolSize = 2000;
		alignas(Get_CacheLineSize()) std::vector<DType*> mResourcePool_Local;

		//static class members have external linkage: same instance shared between every translation unit (would be globally, put only private access)
		alignas(Get_CacheLineSize()) static std::mutex dSharedPoolMutex;
		alignas(Get_CacheLineSize()) static std::vector<DType*> JResourcePool_Shared;
		alignas(Get_CacheLineSize()) static std::size_t dMaxSharedPoolSize;
		alignas(Get_CacheLineSize()) static std::size_t dPoolCounter; //must be accessed within a lock due to how it's used in destructor: freeing all resources
		alignas(Get_CacheLineSize()) static std::atomic<size_t> dObjectCounter; //can be accessed without a lock!
};

/********************************************************************************* DSharedPtrRecycler *********************************************************************************/

template <typename DType> class DSharedPtrRecycler
{
	public:
		DSharedPtrRecycler(void) = delete;
		DSharedPtrRecycler(const std::shared_ptr<JResourcePool<DType>>& sResourcePool) : mResourcePool(sResourcePool) {};
		void operator()(const DType* sResource) const{(*this)(const_cast<DType*>(sResource));}
		void operator()(DType* sResource) const;

	private:
		std::weak_ptr<JResourcePool<DType>> mResourcePool;
};

template <typename DType> void DSharedPtrRecycler<DType>::operator()(DType* sResource) const
{
	auto sSharedPtr = mResourcePool.lock();
	if(sSharedPtr == nullptr)
		delete sResource;
	else
		sSharedPtr->Recycle(sResource);
}

/************************************************************************* STATIC MEMBER DEFINITIONS, STRUCTORS *************************************************************************/

//STATIC MEMBER DEFINITIONS
//Since these are part of a template, these statics will only be defined once, no matter how much this header is included
template <typename DType> std::mutex JResourcePool<DType>::dSharedPoolMutex;
template <typename DType> std::vector<DType*> JResourcePool<DType>::JResourcePool_Shared = {};
template <typename DType> std::size_t JResourcePool<DType>::dMaxSharedPoolSize{1000};
template <typename DType> std::size_t JResourcePool<DType>::dPoolCounter{0};
template <typename DType> std::atomic<std::size_t> JResourcePool<DType>::dObjectCounter{0};

//CONSTRUCTORS
template <typename DType> JResourcePool<DType>::JResourcePool(std::size_t sGetBatchSize, std::size_t sNumToAllocateAtOnce, std::size_t sMaxLocalPoolSize, std::size_t sMaxSharedPoolSize, std::size_t sDebugLevel) : JResourcePool()
{
	Set_ControlParams(sGetBatchSize, sNumToAllocateAtOnce, sMaxLocalPoolSize, sMaxSharedPoolSize, sDebugLevel);
}

template <typename DType> JResourcePool<DType>::JResourcePool(std::size_t sGetBatchSize, std::size_t sNumToAllocateAtOnce, std::size_t sMaxLocalPoolSize) : JResourcePool()
{
	Set_ControlParams(sGetBatchSize, sNumToAllocateAtOnce, sMaxLocalPoolSize);
}

template <typename DType> JResourcePool<DType>::JResourcePool(void)
{
	mResourcePool_Local.reserve(dMaxLocalPoolSize);
	{
		std::lock_guard<std::mutex> sLock(dSharedPoolMutex); //LOCK
		++dPoolCounter;
		if(dDebugLevel > 0)
			std::cout << "CONSTRUCTOR THREAD COUNTER " << typeid(DType).name() << ": " << dPoolCounter << std::endl;
		if(dPoolCounter == 1)
			JResourcePool_Shared.reserve(dMaxSharedPoolSize);
	} //UNLOCK
}

//DESTRUCTOR
template <typename DType> JResourcePool<DType>::~JResourcePool(void)
{
	//Move all objects into the shared pool
	Recycle_Resources_StaticPool(mResourcePool_Local);

	//if this was the last thread, delete all of the remaining resources
	//first move them outside of the vector, then release the lock
	std::vector<DType*> sResources;
	{
		std::lock_guard<std::mutex> sLock(dSharedPoolMutex); //LOCK
		--dPoolCounter;
		if(dDebugLevel > 0)
			std::cout << "DESTRUCTOR THREAD COUNTER " << typeid(DType).name() << ": " << dPoolCounter << std::endl;
		if(dPoolCounter > 0)
			return; //not the last thread

		//last thread: move all resources out of the shared pool
		if(dDebugLevel > 0)
			std::cout << "DESTRUCTOR MOVING FROM SHARED POOL " << typeid(DType).name() << ": " << std::distance(JResourcePool_Shared.begin(), JResourcePool_Shared.end()) << std::endl;
		std::move(JResourcePool_Shared.begin(), JResourcePool_Shared.end(), std::back_inserter(sResources));
		JResourcePool_Shared.clear();
	} //UNLOCK

	//delete the resources
	if(dDebugLevel > 0)
		std::cout << "DESTRUCTOR DELETING " << typeid(DType).name() << ": " << sResources.size() << std::endl;
	for(auto sResource : sResources)
		delete sResource;
	dObjectCounter -= sResources.size(); //I sure hope this is zero!
	if(dDebugLevel > 0)
		std::cout << "All objects (ought) to be destroyed, theoretical # remaining: " << dObjectCounter;
}

/************************************************************************* NON-SHARED-POOL-ACCESSING MEMBER FUNCTIONS *************************************************************************/

template <typename DType> DType* JResourcePool<DType>::Get_Resource(void)
{
	if(dDebugLevel >= 10)
		std::cout << "GET RESOURCE " << typeid(DType).name() << std::endl;
	if(mResourcePool_Local.empty())
		Get_Resources_StaticPool();
	if(mResourcePool_Local.empty())
	{
		//perhaps instead use custom allocator
		auto sPotentialNewSize = mResourcePool_Local.size() + dNumToAllocateAtOnce - 1;
		auto sNumToAllocate = (sPotentialNewSize <= dMaxLocalPoolSize) ? dNumToAllocateAtOnce : (dMaxLocalPoolSize - mResourcePool_Local.size() + 1);
		for(size_t si = 0; si < sNumToAllocate - 1; ++si)
			mResourcePool_Local.push_back(new DType);
		dObjectCounter += sNumToAllocate;
		return new DType();
	}

	auto sResource = mResourcePool_Local.back();
	mResourcePool_Local.pop_back();
	Reset<DType>(sResource);
	return sResource;
}

template <typename DType> void JResourcePool<DType>::Set_ControlParams(size_t sGetBatchSize, size_t sNumToAllocateAtOnce, size_t sMaxLocalPoolSize, size_t sMaxSharedPoolSize, size_t sDebugLevel)
{
	dDebugLevel = sDebugLevel;
	dGetBatchSize = sGetBatchSize;
	dNumToAllocateAtOnce = (sNumToAllocateAtOnce > 0) ? sNumToAllocateAtOnce : 1;
	dMaxLocalPoolSize = sMaxLocalPoolSize;
	{
		std::lock_guard<std::mutex> sLock(dSharedPoolMutex); //LOCK
		dMaxSharedPoolSize = sMaxSharedPoolSize;
		JResourcePool_Shared.reserve(dMaxSharedPoolSize);
	} //UNLOCK
}

template <typename DType> void JResourcePool<DType>::Set_ControlParams(size_t sGetBatchSize, size_t sNumToAllocateAtOnce, size_t sMaxLocalPoolSize)
{
	dGetBatchSize = sGetBatchSize;
	dNumToAllocateAtOnce = (sNumToAllocateAtOnce > 0) ? sNumToAllocateAtOnce : 1;
	dMaxLocalPoolSize = sMaxLocalPoolSize;
}

template <typename DType> std::shared_ptr<DType> JResourcePool<DType>::Get_SharedResource(void)
{
	return shared_ptr<DType>(Get_Resource(), DSharedPtrRecycler<DType>(this->shared_from_this()));
}

template <typename DType> void JResourcePool<DType>::Recycle(std::vector<const DType*>& sResources)
{
	std::vector<DType*> sNonConstResources;
	sNonConstResources.reserve(sResources.size());

	auto Deconstifier = [](const DType* sConstPointer) -> DType* {return const_cast<DType*>(sConstPointer);};
	std::transform(sResources.begin(), sResources.end(), std::back_inserter(sNonConstResources), Deconstifier);
	sResources.clear();

	Recycle(sNonConstResources);
}

template <typename DType> void JResourcePool<DType>::Recycle(std::vector<DType*>& sResources)
{
	for(auto& sResource : sResources)
		Release_Resources<DType>(sResource);

	size_t sFirstToMoveIndex = 0;
	auto sPotentialNewPoolSize = mResourcePool_Local.size() + sResources.size();
	if(sPotentialNewPoolSize > dMaxLocalPoolSize) //we won't move all of the resources into the sal pool, as it would be too large: only move a subset
		sFirstToMoveIndex = sResources.size() - (dMaxLocalPoolSize - mResourcePool_Local.size());

	std::move(sResources.begin() + sFirstToMoveIndex, sResources.end(), std::back_inserter(mResourcePool_Local));
	sResources.resize(sFirstToMoveIndex);
	if(!sResources.empty())
		Recycle_Resources_StaticPool(sResources);
}

template <typename DType> void JResourcePool<DType>::Recycle(DType* sResource)
{
	if(sResource == nullptr)
		return;
	std::vector<DType*> sResourceVector{sResource};
	Recycle(sResourceVector);
}

/************************************************************************* SHARED-POOL-ACCESSING MEMBER FUNCTIONS *************************************************************************/

template <typename DType> void JResourcePool<DType>::Get_Resources_StaticPool(void)
{
	std::size_t sPotentialNewLocalSize, sGetBatchSize;
	if(dMaxLocalPoolSize == 0) //Special case
	{
		sGetBatchSize = 1;
		sPotentialNewLocalSize = 1;
	}
	else
	{
		sPotentialNewLocalSize = mResourcePool_Local.size() + dGetBatchSize;
		sGetBatchSize = (sPotentialNewLocalSize <= dMaxLocalPoolSize) ? dGetBatchSize : dMaxLocalPoolSize - mResourcePool_Local.size();
	}


	{
		std::lock_guard<std::mutex> sLock(dSharedPoolMutex); //LOCK
		if(JResourcePool_Shared.empty())
			return;
		auto sFirstToMoveIndex = (sGetBatchSize >= JResourcePool_Shared.size()) ? 0 : JResourcePool_Shared.size() - sGetBatchSize;
		if(dDebugLevel > 0)
			std::cout << "MOVING FROM SHARED POOL " << typeid(DType).name() << ": " << JResourcePool_Shared.size() - sFirstToMoveIndex << std::endl;
		std::move(JResourcePool_Shared.begin() + sFirstToMoveIndex, JResourcePool_Shared.end(), std::back_inserter(mResourcePool_Local));
		JResourcePool_Shared.resize(sFirstToMoveIndex);
	} //UNLOCK
}

template <typename DType> void JResourcePool<DType>::Recycle_Resources_StaticPool(std::vector<DType*>& sResources)
{
	std::size_t sFirstToMoveIndex = 0;
	{
		std::lock_guard<std::mutex> sLock(dSharedPoolMutex); //LOCK

		auto sPotentialNewPoolSize = JResourcePool_Shared.size() + sResources.size();
		if(sPotentialNewPoolSize > dMaxSharedPoolSize) //we won't move all of the resources into the shared pool, as it would be too large: only move a subset
			sFirstToMoveIndex = sResources.size() - (dMaxSharedPoolSize - JResourcePool_Shared.size());

		if(dDebugLevel > 0)
			std::cout << "MOVING TO SHARED POOL " << typeid(DType).name() << ": " << mResourcePool_Local.size() - sFirstToMoveIndex << std::endl;

		std::move(sResources.begin() + sFirstToMoveIndex, sResources.end(), std::back_inserter(JResourcePool_Shared));
	} //UNLOCK

	if(dDebugLevel > 0)
		std::cout << "DELETING " << typeid(DType).name() << ": " << sFirstToMoveIndex << std::endl;

	//any resources that were not moved into the shared pool are deleted instead (too many)
	auto Deleter = [](DType* sResource) -> void {delete sResource;};
	std::for_each(sResources.begin(), sResources.begin() + sFirstToMoveIndex, Deleter);

	dObjectCounter -= sFirstToMoveIndex;
	sResources.clear();
}

template <typename DType> std::size_t JResourcePool<DType>::Get_SharedPoolSize(void) const
{
	std::lock_guard<std::mutex> sLock(dSharedPoolMutex); //LOCK
	return JResourcePool_Shared.size();
} //UNLOCK

#endif // JResourcePool_h
