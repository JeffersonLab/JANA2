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
#include <iterator>

#include "JResettable.h"

/****************************************************** OVERVIEW ******************************************************
 *
 * This class can be used as a wrapper to a (private) static resource pool that is accessible to all class instances.
 * A pool counter keeps track of how many JResourcePool's there are for a given type.
 * Once it drops to zero, all of the remaining objects are deleted.
 *
 ***************************************** DEFINITION AND USE: NON-SHARED_PTR'S ***************************************
 *
 * You can retrieve and recycle resources via the Get_Resource() and Recycle() functions.
 * Be sure to recycle the memory once you are done with it or else it will leak.
 *
 ******************************************* DEFINITION AND USE: SHARED_PTR'S *****************************************
 *
 * You can retrieve shared_ptr's of the objects by calling the Get_SharedResource() method.
 * The advantage of using shared_ptr's is that they automatically keep track of when they are out of scope.
 * These shared_ptr's have been created with a JSharedPtrRecycler functor:
 * Once the shared_ptr goes out of scope, the contained resource is automatically recycled back to the pool.
 *
 * The shared_ptr MUST NOT outlive the life of the JResourcePool!!! Or else it will crash.
 *
 **********************************************************************************************************************/

// Apple compiler does not currently support alignas. Make this an empty definition if it is not already defined.
#ifndef alignas
#define alignas(A)
#endif

template <typename DType> class JResourcePool
{
	//TYPE TRAIT REQUIREMENTS
	//If these statements are false, this won't compile
	static_assert(!std::is_pointer<DType>::value, "The template type for JResourcePool must not be a pointer (the stored type IS a pointer though).");
	static_assert(!std::is_const<DType>::value, "The template type for JResourcePool must not be const.");
	static_assert(!std::is_volatile<DType>::value, "The template type for JResourcePool must not be volatile.");

	public:
		//STRUCTORS
		JResourcePool(void);
		JResourcePool(std::size_t sMaxPoolSize, std::size_t sDebugLevel);
		~JResourcePool(void);

		//COPIERS
		JResourcePool(const JResourcePool&) = delete;
		JResourcePool& operator=(const JResourcePool&) = delete;

		//MOVERS
		JResourcePool(JResourcePool&&) = delete;
		JResourcePool& operator=(JResourcePool&&) = delete;

		void Set_ControlParams(std::size_t sMaxPoolSize, std::size_t sDebugLevel);
		std::size_t Get_MaxPoolSize(void){ return dMaxPoolSize; }

		//GET RESOURCES
		template <typename... ConstructorArgTypes>
		DType* Get_Resource(ConstructorArgTypes&&... aConstructorArgs);
		template <typename ContainerType, typename... ConstructorArgTypes>
		void Get_Resources(std::size_t aNumResources, std::back_insert_iterator<ContainerType> aInsertIterator, ConstructorArgTypes&&... aConstructorArgs);

		//GET SHARED RESOURCES
		template <typename... ConstructorArgTypes>
		std::shared_ptr<DType> Get_SharedResource(ConstructorArgTypes&&... aConstructorArgs);
		template <typename ContainerType, typename... ConstructorArgTypes>
		void Get_SharedResources(std::size_t aNumResources, std::back_insert_iterator<ContainerType> aInsertIterator, ConstructorArgTypes&&... aConstructorArgs);

		//RECYCLE CONST OBJECTS //these methods just const_cast and call the non-const versions
		void Recycle(const DType* sResource){Recycle(const_cast<DType*>(sResource));}
		void Recycle(std::vector<const DType*>& sResources); //move-clears the input vector

		//RECYCLE NON-CONST OBJECTS
		void Recycle(DType* sResource);
		void Recycle(std::vector<DType*>& sResources); //move-clears the input vector

		std::size_t Get_PoolSize(void) const;
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

		void LockPool(void) const;

		//Enable this version if type inherits from JResettable //void: is return type
		template <typename RType> typename std::enable_if<std::is_base_of<JResettable, RType>::value, void>::type Release_Resources(JResettable* sResource){sResource->Release();}
		template <typename RType> typename std::enable_if<std::is_base_of<JResettable, RType>::value, void>::type Reset(JResettable* sResource){sResource->Reset();}

		//Enable this version if type does NOT inherit from JResettable //void: is return type
		template <typename RType> typename std::enable_if<!std::is_base_of<JResettable, RType>::value, void>::type Release_Resources(RType* sResource){};
		template <typename RType> typename std::enable_if<!std::is_base_of<JResettable, RType>::value, void>::type Reset(RType* sResource){};

		template <typename ContainerType>
		std::size_t Get_Resources_StaticPool(std::size_t aNumResources, std::back_insert_iterator<ContainerType> aInsertIterator);
		DType* Get_Resource_StaticPool(void);
		void Recycle_Resources_StaticPool(std::vector<DType*>& sResources);
		void Recycle_Resource_StaticPool(DType* sResource);

		alignas(Get_CacheLineSize()) std::size_t dDebugLevel = 0;

		//static class members have external linkage: same instance shared between every translation unit (would be globally, put only private access)
		alignas(Get_CacheLineSize()) static std::atomic<bool> dPoolLock;
		alignas(Get_CacheLineSize()) static std::vector<DType*> mResourcePool;
		alignas(Get_CacheLineSize()) static std::size_t dMaxPoolSize;
		alignas(Get_CacheLineSize()) static std::size_t dPoolCounter; //must be accessed within a lock due to how it's used in destructor: freeing all resources
		alignas(Get_CacheLineSize()) static std::atomic<size_t> dObjectCounter; //can be accessed without a lock!
};

/********************************************************************************* JSharedPtrRecycler *********************************************************************************/

template <typename DType> class JSharedPtrRecycler
{
	public:
		JSharedPtrRecycler(void) = delete;
		JSharedPtrRecycler(JResourcePool<DType>* sResourcePool) : mResourcePool(sResourcePool) {};
		void operator()(const DType* sResource) const{(*this)(const_cast<DType*>(sResource));}
		void operator()(DType* sResource) const{mResourcePool->Recycle(sResource);};

	private:
		JResourcePool<DType>* mResourcePool = nullptr;
};

/************************************************************************* STATIC MEMBER DEFINITIONS, STRUCTORS *************************************************************************/

//STATIC MEMBER DEFINITIONS
//Since these are part of a template, these statics will only be defined once, no matter how much this header is included
template <typename DType> std::atomic<bool> JResourcePool<DType>::dPoolLock{0};
template <typename DType> std::vector<DType*> JResourcePool<DType>::mResourcePool = {};
template <typename DType> std::size_t JResourcePool<DType>::dMaxPoolSize{10};
template <typename DType> std::size_t JResourcePool<DType>::dPoolCounter{0};
template <typename DType> std::atomic<std::size_t> JResourcePool<DType>::dObjectCounter{0};

//CONSTRUCTORS
template <typename DType> JResourcePool<DType>::JResourcePool(std::size_t sMaxPoolSize, std::size_t sDebugLevel) : JResourcePool()
{
	Set_ControlParams(sMaxPoolSize, sDebugLevel);
}

template <typename DType> JResourcePool<DType>::JResourcePool(void)
{
	LockPool();
	++dPoolCounter;
	if(dDebugLevel > 0)
		std::cout << "CONSTRUCTOR POOL COUNTER " << typeid(DType).name() << ": " << dPoolCounter << std::endl;
	if(dPoolCounter == 1)
		mResourcePool.reserve(dMaxPoolSize);
	dPoolLock = false; //unlock
}

//DESTRUCTOR
template <typename DType> JResourcePool<DType>::~JResourcePool(void)
{
	//if this was the last thread, delete all of the remaining resources
	//first move them outside of the vector, then release the lock
	std::vector<DType*> sResources;

	//Lock
	LockPool();

	//Update/check pool counter
	auto sNumPoolsRemaining = --dPoolCounter;
	if(dDebugLevel > 0)
		std::cout << "DESTRUCTOR POOL COUNTER " << typeid(DType).name() << ": " << sNumPoolsRemaining << std::endl;
	if(dPoolCounter > 0)
	{
		dPoolLock = false; //unlock
		return; //not the last thread
	}

	//last thread: move all resources out of the shared pool
	if(dDebugLevel > 0)
		std::cout << "DESTRUCTOR GETTING FROM SHARED POOL " << typeid(DType).name() << ": " << std::distance(mResourcePool.begin(), mResourcePool.end()) << std::endl;
	std::copy(mResourcePool.begin(), mResourcePool.end(), std::back_inserter(sResources));
	mResourcePool.clear();
	dPoolLock = false; //unlock

	//delete the resources
	if(dDebugLevel > 0)
		std::cout << "DESTRUCTOR DELETING " << typeid(DType).name() << ": " << sResources.size() << std::endl;
	for(auto sResource : sResources)
		delete sResource;
	dObjectCounter -= sResources.size(); //I sure hope this is zero!
	if(dDebugLevel > 0)
		std::cout << "All objects (ought) to be destroyed, theoretical # remaining: " << dObjectCounter << "\n";
}

/************************************************************************* NON-SHARED-POOL-ACCESSING MEMBER FUNCTIONS *************************************************************************/

template <typename DType> void JResourcePool<DType>::Set_ControlParams(size_t sMaxPoolSize, size_t sDebugLevel)
{
	dDebugLevel = sDebugLevel;

	LockPool();
	dMaxPoolSize = sMaxPoolSize;
	mResourcePool.reserve(dMaxPoolSize);
	dPoolLock = false;
}

template <typename DType>
template <typename... ConstructorArgTypes>
DType* JResourcePool<DType>::Get_Resource(ConstructorArgTypes&&... aConstructorArgs)
{
	if(dDebugLevel >= 10)
		std::cout << "GET RESOURCE " << typeid(DType).name() << std::endl;

	//Get resources from pool
	auto sResource = Get_Resource_StaticPool();
	if(sResource != nullptr)
		return sResource;

	//Pool was empty: Allocate one
	dObjectCounter++;
	return new DType(std::forward<ConstructorArgTypes>(aConstructorArgs)...);
}

template <typename DType>
template <typename ContainerType, typename... ConstructorArgTypes>
void JResourcePool<DType>::Get_Resources(std::size_t aNumResources, std::back_insert_iterator<ContainerType> aInsertIterator, ConstructorArgTypes&&... aConstructorArgs)
{
	if(dDebugLevel >= 10)
		std::cout << "GET " << aNumResources << " RESOURCES " << typeid(DType).name() << std::endl;

	//Get resources from pool
	auto sNumAcquired = Get_Resources_StaticPool(aNumResources, aInsertIterator);

	//Allocate the rest (if the pool didn't have enough)
	dObjectCounter += aNumResources - sNumAcquired; //Amount we are about to allocate
	while(sNumAcquired != aNumResources)
	{
		aInsertIterator = new DType(std::forward<ConstructorArgTypes>(aConstructorArgs)...);
		sNumAcquired++;
	}
}

template <typename DType>
template <typename... ConstructorArgTypes>
std::shared_ptr<DType> JResourcePool<DType>::Get_SharedResource(ConstructorArgTypes&&... aConstructorArgs)
{
	return std::shared_ptr<DType>(Get_Resource(std::forward<ConstructorArgTypes>(aConstructorArgs)...), JSharedPtrRecycler<DType>(this));
}

template <typename DType>
template <typename ContainerType, typename... ConstructorArgTypes>
void JResourcePool<DType>::Get_SharedResources(std::size_t aNumResources, std::back_insert_iterator<ContainerType> aInsertIterator, ConstructorArgTypes&&... aConstructorArgs)
{
	//Get unshared resources
	std::vector<DType*> sResources;
	sResources.reserve(aNumResources);
	Get_Resources(aNumResources, std::back_inserter(sResources), std::forward<ConstructorArgTypes>(aConstructorArgs)...);

	//Convert to shared_ptr's
	for(auto sResource : sResources)
		aInsertIterator = std::shared_ptr<DType>(sResource, JSharedPtrRecycler<DType>(this)); //calls push_back with r-value (doesn't change ref count)
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

	Recycle_Resources_StaticPool(sResources);
}

template <typename DType> void JResourcePool<DType>::Recycle(DType* sResource)
{
	if(sResource == nullptr)
		return;
	Release_Resources<DType>(sResource);
	Recycle_Resource_StaticPool(sResource);
}

/************************************************************************* SHARED-POOL-ACCESSING MEMBER FUNCTIONS *************************************************************************/

//---------------------------------
// LockPool
//---------------------------------
template <typename DType>
void JResourcePool<DType>::LockPool(void) const
{
	bool sExpected = false;
	while(!dPoolLock.compare_exchange_weak(sExpected, true))
		sExpected = false;
}

template <typename DType>
template <typename ContainerType>
std::size_t JResourcePool<DType>::Get_Resources_StaticPool(std::size_t aNumResources, std::back_insert_iterator<ContainerType> aInsertIterator)
{
	//Lock
	LockPool();

	//Return if empty
	if(mResourcePool.empty())
	{
		dPoolLock = false; //Unlock
		return 0;
	}

	//Get resources from the back of the pool
	//Beware, we may be requesting more resources than are in the pool
	auto sFirstToCopyIndex = (aNumResources >= mResourcePool.size()) ? 0 : mResourcePool.size() - aNumResources;
	std::copy(mResourcePool.begin() + sFirstToCopyIndex, mResourcePool.end(), aInsertIterator);
	auto sNumRetrieved = mResourcePool.size() - sFirstToCopyIndex;
	mResourcePool.resize(sFirstToCopyIndex);

	dPoolLock = false; //Unlock
	if(dDebugLevel > 0)
		std::cout << "RETRIEVED FROM SHARED POOL " << typeid(DType).name() << ": " << sNumRetrieved << std::endl;

	return sNumRetrieved;
}

template <typename DType>
DType* JResourcePool<DType>::Get_Resource_StaticPool(void)
{
	DType* sResource = nullptr;

	//Lock
	LockPool();

	//Return if empty
	if(mResourcePool.empty())
	{
		dPoolLock = false; //Unlock
		return nullptr;
	}

	//Get resource from the back of the pool
	sResource = mResourcePool.back();
	mResourcePool.pop_back();

	dPoolLock = false; //Unlock
	if(dDebugLevel > 0)
		std::cout << "RETRIEVED FROM SHARED POOL " << typeid(DType).name() << ": " << 1 << std::endl;

	return sResource;
}

template <typename DType>
void JResourcePool<DType>::Recycle_Resources_StaticPool(std::vector<DType*>& sResources)
{
	if(dDebugLevel >= 10)
		std::cout << "RECYCLE " << sResources.size() << " RESOURCES " << typeid(DType).name() << std::endl;

	std::size_t sFirstToCopyIndex = 0;
	{
		LockPool();

		auto sPotentialNewPoolSize = mResourcePool.size() + sResources.size();
		if(sPotentialNewPoolSize > dMaxPoolSize) //we won't move all of the resources into the shared pool, as it would be too large: only move a subset
			sFirstToCopyIndex = sResources.size() - (dMaxPoolSize - mResourcePool.size());

		std::copy(sResources.begin() + sFirstToCopyIndex, sResources.end(), std::back_inserter(mResourcePool));
		dPoolLock = false; //Unlock
	}

	if(dDebugLevel > 0)
	{
		std::cout << "PUT IN SHARED POOL " << typeid(DType).name() << ": " << sResources.size() - sFirstToCopyIndex << std::endl;
		std::cout << "DELETING " << typeid(DType).name() << ": " << sFirstToCopyIndex << std::endl;
	}

	//any resources that were not copied into the shared pool are deleted instead (too many)
	auto Deleter = [](DType* sResource) -> void {delete sResource;};
	std::for_each(sResources.begin(), sResources.begin() + sFirstToCopyIndex, Deleter);

	dObjectCounter -= sFirstToCopyIndex;
	sResources.clear();
}

template <typename DType>
void JResourcePool<DType>::Recycle_Resource_StaticPool(DType* sResource)
{
	if(dDebugLevel >= 10)
		std::cout << "RECYCLE 1 RESOURCE " << typeid(DType).name() << std::endl;

	{
		LockPool();

		if(mResourcePool.size() != dMaxPoolSize)
		{
			mResourcePool.push_back(sResource);
			sResource = nullptr;
		}

		dPoolLock = false; //Unlock
	}

	if(dDebugLevel > 10)
	{
		std::cout << "PUT IN SHARED POOL " << typeid(DType).name() << ": " << (sResource == nullptr) << std::endl;
		std::cout << "DELETING " << typeid(DType).name() << ": " << (sResource != nullptr) << std::endl;
	}

	//any resources that were not copied into the shared pool are deleted instead (too many)

	if(sResource != nullptr)
	{
		delete sResource;
		dObjectCounter--;
	}
}

template <typename DType> std::size_t JResourcePool<DType>::Get_PoolSize(void) const
{
	LockPool();
	auto sSize = mResourcePool.size();
	dPoolLock = false; //Unlock
	return sSize;
}

#endif // JResourcePool_h
