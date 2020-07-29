
// Copyright 2020, Jefferson Science Associates, LLC.
// Subject to the terms in the LICENSE file found in the top-level directory.

#ifndef _JFactorySet_h_
#define _JFactorySet_h_

#include <string>
#include <typeindex>
#include <map>

#include <JANA/JFactoryT.h>
#include <JANA/Utils/JResettable.h>
#include <JANA/Status/JComponentSummary.h>

class JFactoryGenerator;
class JFactory;


class JFactorySet : public JResettable
{
	public:
		JFactorySet(void);
		JFactorySet(const std::vector<JFactoryGenerator*>& aFactoryGenerators);
		virtual ~JFactorySet();
		
		bool Add(JFactory* aFactory);
		void Merge(JFactorySet &aFactorySet);
		void Print(void) const;
		void Release(void);

		JFactory* GetFactory(std::type_index aObjectType, const std::string& aFactoryTag="") const;
		template<typename T> JFactoryT<T>* GetFactory(const std::string& tag = "") const;
        std::vector<JFactory*> GetAll() const;
		template<typename T> std::vector<JFactoryT<T>*> GetFactoryAll() const;

		std::vector<JFactorySummary> Summarize() const;

	protected:
	
		//string: tag
		std::map<std::pair<std::type_index, std::string>, JFactory*> mFactories;
};

template<typename T>
JFactoryT<T>* JFactorySet::GetFactory(const std::string& tag) const {

	auto sKeyPair = std::make_pair(std::type_index(typeid(T)), tag);
	auto sIterator = mFactories.find(sKeyPair);
	return (sIterator != std::end(mFactories)) ? static_cast<JFactoryT<T>*>(sIterator->second) : nullptr;
}

template<typename T>
std::vector<JFactoryT<T>*> JFactorySet::GetFactoryAll() const {
	auto sKey = std::type_index(typeid(T));
	std::vector<JFactoryT<T>*> data;
	for (auto it=std::begin(mFactories);it!=std::end(mFactories);it++){
		if (it->first.first==sKey){
			data.push_back(static_cast<JFactoryT<T>*>(it->second));
		}
	}
	return data;
}


#endif // _JFactorySet_h_

