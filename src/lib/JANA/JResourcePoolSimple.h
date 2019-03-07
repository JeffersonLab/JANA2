#pragma once

#include <map>
#include <mutex>
#include <thread>

#include <JANA/JLogger.h>

template <typename T>
class JResourcePoolSimple {

	typedef std::thread::id bucket_t;

	private:

	std::multimap<bucket_t, T*> mPool;
	std::mutex mMutex;
	size_t mCheckedOut = 0;
	size_t mPoolSize = 0;
	size_t mMaxPoolSize;
	std::shared_ptr<JLogger> mLogger;


	bucket_t Get_Bucket() {
		return std::this_thread::get_id();
	}


	public:

	JResourcePoolSimple(size_t aMaxPoolSize = 16) 
		: mMaxPoolSize(aMaxPoolSize), mLogger(new JLogger) {

		mLogger->level = JLogLevel::DEBUG;
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
		LOG_INFO(mLogger) << "JResourcePoolSimple<" << typeid(T).name() 
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
			      << "JResourcePoolSimple<" << typeid(T).name() << ">::Get_Resource: " 
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
			      << "JResourcePoolSimple<" << typeid(T).name() << ">::Get_Resource: " 
			      << "Creating new resource; bucket=" << bucket << LOG_END;

			++mCheckedOut;
			return new T(std::forward<CtorArgTypes>(args)...);
		}
	}

	void Recycle(T* resource) {

		mMutex.lock();

		if (mPoolSize == mMaxPoolSize) {  

			// Pool is already full
			--mCheckedOut;

			mMutex.unlock();
			delete resource;

			LOG_DEBUG(mLogger) 
				<< "JResourcePoolSimple<" << typeid(T).name() << ">::Recycle: " 
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
				<< "JResourcePoolSimple<" << typeid(T).name() << ">::Recycle: " 
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




