#pragma once

#include <map>
#include <mutex>
#include <thread>

#include <JANA/JLogger.h>
#include <JANA/JCpuInfo.h>
#include <JANA/JTypeInfo.h>


template <typename T>
class JResourcePoolSimple {

	typedef int bucket_t;

	private:

	std::multimap<bucket_t, T*> mPool;
	std::mutex mMutex;
	size_t mCheckedOut = 0;
	size_t mPoolSize = 0;
	size_t mMaxPoolSize;
	JLogger mLogger;


	inline bucket_t Get_Bucket() {
		return JCpuInfo::GetNumaNodeID();
	}


	public:

	JResourcePoolSimple(size_t aMaxPoolSize = 16) : mMaxPoolSize(aMaxPoolSize) {
	    mLogger = JLoggingService::logger("JResourcePoolSimple");
	}

	JResourcePoolSimple(const JResourcePoolSimple& other) = delete;

	JResourcePoolSimple& operator=(const JResourcePoolSimple& other) = delete;

	JResourcePoolSimple(JResourcePoolSimple&& other) = delete;

	JResourcePoolSimple& operator=(JResourcePoolSimple&& other) = delete;

	~JResourcePoolSimple() {
		
		std::lock_guard<std::mutex> lock(mMutex);
		size_t deleteCount = 0;

		for (auto item : mPool) {
			++deleteCount;
			delete item.second;
		}
		mPool.clear();
    LOG_INFO(mLogger) << "JResourcePoolSimple<" << JTypeInfo::demangle<T>()
			   << ">::~JResourcePool: " << "Deleted " << deleteCount 
			   << " items (" << mPoolSize << " expected)." << LOG_END;
	}


	template <typename... CtorArgTypes>
	T* Get_Resource(CtorArgTypes&&... args) {

		auto bucket = Get_Bucket();
		std::lock_guard<std::mutex> lock(mMutex);
		auto search = mPool.find(bucket);

		if (search != mPool.end()) {
			LOG_DEBUG(mLogger)
        << "JResourcePoolSimple<" << JTypeInfo::demangle<T>() << ">::Get_Resource: "
			  << "Acquired resource from bucket=" << bucket
			  << LOG_END;

			T* result = search->second;
			mPool.erase(search);
			++mCheckedOut;
			--mPoolSize;
			return result;
		}
		else {
			LOG_DEBUG(mLogger)
        << "JResourcePoolSimple<" << JTypeInfo::demangle<T>() << ">::Get_Resource: "
			  << "Creating new resource; bucket=" << bucket << LOG_END;

			++mCheckedOut;
			return new T(std::forward<CtorArgTypes>(args)...);
		}
	}

	void Recycle(T* resource) {

    resource->Release();
    // TODO: Reconsider this
    // This resource release is what "clears" a factory after an Event
    // is finished. However, not all data is meant to be cleared after
    // each event. There are two phases of clearing data in a factory:
    // change of event number, and change of run number. Neither of these
    // really correspond to the concept of "releasing the
    // factory's resources"

		mMutex.lock();

		if (mPoolSize == mMaxPoolSize) {  

			// Pool is already full
			--mCheckedOut;

			mMutex.unlock();
			delete resource;

			LOG_DEBUG(mLogger) 
				<< "JResourcePoolSimple<" << JTypeInfo::demangle<T>() << ">::Recycle: " 
				<< "Pool is full; deleting resource." << LOG_END;

			return;
		} 
		else { 
			// Return to pool
			auto bucket = Get_Bucket();
			mPool.insert({bucket, resource});
			++mPoolSize;
			--mCheckedOut;
			mMutex.unlock();

			LOG_DEBUG(mLogger) 
				<< "JResourcePoolSimple<" << JTypeInfo::demangle<T>() << ">::Recycle: " 
			  << "Returning resource to pool; bucket=" << bucket << LOG_END;

			return;
		}
	}

	void Set_ControlParams(size_t aMaxPoolSize, int aDebugLevel) {
		mMaxPoolSize = aMaxPoolSize;
	}

	size_t Get_PoolSize() {
		std::lock_guard<std::mutex> lock(mMutex);
		return mPoolSize;
	}

	size_t Get_MaxPoolSize() {
		std::lock_guard<std::mutex> lock(mMutex);
		return mMaxPoolSize;
	}

};




