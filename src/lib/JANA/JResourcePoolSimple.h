#pragma once

#include <map>
#include <mutex>
#include <thread>

template <typename T>
class JResourcePoolSimple {

	typedef std::thread::id bucket_t;

	private:

	std::multimap<bucket_t, T*> mPool;
	std::mutex mMutex;
	size_t mCheckedOut = 0;
	size_t mPoolSize = 0;
	size_t mMaxPoolSize;


	bucket_t Get_Bucket() {
		return std::this_thread::get_id();
	}


	public:

	JResourcePoolSimple(size_t aMaxPoolSize = 16) : mMaxPoolSize(aMaxPoolSize) {}

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
		jout << "JResourcePoolSimple<" << typeid(T).name() << ">::~JResourcePool: " 
		     << "Deleted " << deleteCount << " items (" << mPoolSize << " expected)." 
		     << std::endl;
	}


	template <typename... CtorArgTypes>
	T* Get_Resource(CtorArgTypes&&... args) {

		auto bucket = Get_Bucket();
		std::lock_guard<std::mutex> lock(mMutex);
		auto search = mPool.find(bucket);

		if (search != mPool.end()) {
			jout << "JResourcePoolSimple<" << typeid(T).name() << "::Get_Resource: " 
			     << "Returning pre-existing resource from pool for bucket " << bucket 
			     << std::endl;

			T* result = search->second;
			mPool.erase(search);
			++mCheckedOut;
			--mPoolSize;
			return result;
		}
		else {
			jout << "JResourcePoolSimple<" << typeid(T).name() << "::Get_Resource: " 
			     << "Creating new resource" << std::endl;

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

			jout << "JResourcePoolSimple<" << typeid(T).name() << "::Recycle: " 
			     << "Pool is already full. Deleting resource outright." << std::endl;

			return;
		} 
		else { 
			// Return to pool
			auto bucket = Get_Bucket();
			mPool.insert({bucket, resource});
			++mPoolSize;
			--mCheckedOut;
			mMutex.unlock();

			jout << "JResourcePoolSimple<" << typeid(T).name() << "::Recycle: " 
			     << "Returning resource to pool for bucket " << bucket << std::endl;

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




