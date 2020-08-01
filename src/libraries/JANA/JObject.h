
// Copyright 2020, Jefferson Science Associates, LLC.
// Subject to the terms in the LICENSE file found in the top-level directory.

#ifndef _JObject_h_
#define _JObject_h_

#include <string>
#include <sstream>
#include <set>
#include <vector>
#include <cassert>
#include <typeinfo>

#include <JANA/Utils/JTypeInfo.h>
#include <JANA/JLogger.h>
#include <JANA/JException.h>

/// The JObject class is a base class for all data classes.
/// (See JFactory for algorithm classes.)

/// The JOBJECT_PUBLIC macro is used to generate the correct className() overrides for a JObject subclass
#define JOBJECT_PUBLIC(T) \
	static const std::string static_className() {return #T;} \
	const std::string className() const override {return #T;} \


struct JObjectMember {
    /// A plain-old data structure for describing one member of a JObject
	std::string name;        // E.g. "E_tot"
	std::string type;        // E.g. "float"
	std::string value;       // E.g. "22.2e-2"
	std::string description; // E.g. "GeV"
};

class JObjectSummary {
	/// A container data structure for collecting information about JObjects. This is meant to be
	/// used by tools for debugging/visualizing/introspecting. The information for each member is
	/// collected by JObject::Summarize().

	std::vector<JObjectMember> m_fields;

public:
	/// get_fields() returns a copy of all JObjectMember information collected so far.
	std::vector<JObjectMember> get_fields() const {
		return m_fields;
	}

	/// add() is used to insert a new row of JObjectMember data
	template <typename T>
	void add(const T& x, const char* name, const char* format, const char* description="") {

		char buffer[256];
		snprintf(buffer, 256, format, x);
		m_fields.push_back({name, JTypeInfo::builtin_typename<T>(), buffer, description});
	}
};

class JObject{
	public:
		JObject() = default;
		virtual ~JObject() = default;

		virtual const std::string className() const {
		    /// className returns a string representation of the name of this class.
		    /// This won't automatically do the right thing -- in each JObject subclass,
		    /// the user either needs to use the JOBJECT_PUBLIC macro,
		    /// or override this method while keeping the same method body.
			return JTypeInfo::demangle<decltype(*this)>();
		}
	
		// Associated objects
		inline void AddAssociatedObject(const JObject *obj);
		inline void AddAssociatedObjectAutoDelete(JObject *obj, bool auto_delete=true);
		inline void RemoveAssociatedObject(const JObject *obj);
		inline void ClearAssociatedObjects(void);
		inline bool IsAssociated(const JObject* locObject) const {return (associated.find(locObject) != associated.end());}

		template<class T> const T* GetSingle() const;
		template<class T> std::vector<const T*> Get() const;



		// Convert to strings with pretty formatting for printing
		virtual void Summarize(JObjectSummary& summary) const;

	protected:
		std::set<const JObject*> associated;
		std::set<JObject*> auto_delete;
};


//--------------------------
// AddAssociatedObject
//--------------------------
void JObject::AddAssociatedObject(const JObject *obj)
{
	/// Add a JObject to the list of associated objects
	assert(obj!=NULL);
	associated.insert(obj);
}

//--------------------------
// AddAssociatedObjectAutoDelete
//--------------------------
void JObject::AddAssociatedObjectAutoDelete(JObject *obj, bool auto_delete)
{
	/// Add a JObject to the list of associated objects. If the auto_delete
	/// flag is true, then automatically delete it when this object is
	/// deleted. Otherwise, this behaves identically to the AddAssociatedObject
	/// method.
	///
	/// Note that if the object is removed via RemoveAssociatedObject(...)
	/// then the object is NOT deleted. BUT, if the entire list of associated
	/// objects is cleared via ClearAssociatedObjects, then the object will
	/// be deleted.

	AddAssociatedObject(obj);
	if(auto_delete) this->auto_delete.insert(obj);
}

//--------------------------
// RemoveAssociatedObject
//--------------------------
void JObject::RemoveAssociatedObject(const JObject *obj)
{
	/// Remove the specified JObject from the list of associated
	/// objects. This will NOT delete the object even if the
	/// object was added with the AddAssociatedObjectAutoDelete(...)
	/// method with the auto_delete flag set.

	auto iter = associated.find(obj);
	
	if(iter!=associated.end()){
		associated.erase(iter);
	}
}

//--------------------------
// ClearAssociatedObjects
//--------------------------
void JObject::ClearAssociatedObjects(void)
{
	/// Remove all associated objects from the associated objects list.
	/// This will also delete any objects that were added via the
	/// AddAssociatedObjectAutoDelete(...) method with the auto_delete
	/// flag set.
	
	// Clear pointers to associated objects
	associated.clear();
	
	// Delete objects in the auto_delete list
	for( auto p : auto_delete ) delete p;
	auto_delete.clear();
}


template<class T>
const T* JObject::GetSingle() const {
	/// This is a convenience method that can be used to get a pointer to the single associated object of type T.
	/// If no object is found, this will return nullptr.
	/// If multiple objects are found, this will throw a JException.

	const T* last_found = nullptr;
	for (auto obj : associated) {
		auto t = dynamic_cast<const T*>(obj);
		if (t != nullptr) {
			if (last_found == nullptr) {
				last_found = t;
			}
			else {
				throw JException("JObject::GetSingle(): Multiple objects found.");
			}
		}
	}
	if (last_found == nullptr) {
		// If nothing found, attempt matching by strings.
		// Why? Because dl and RTTI aren't playing nicely together;
		// each plugin might assign a different typeid to the same class.
		std::string classname = JTypeInfo::demangle<T>();
		for (auto obj : associated) {
			if (obj->className() == classname) {
				if (last_found == nullptr) {
					last_found = reinterpret_cast<const T*>(obj);
				}
				else {
					throw JException("JObject::GetSingle(): Multiple objects found.");
				}
			}
		}
	}
	return last_found;
}

template<typename T>
std::vector<const T*> JObject::Get() const {
	/// Returns a vector of pointers to all associated objects of type T.

	std::vector<const T*> results;
	for (auto obj : associated) {
		const T* t = dynamic_cast<const T*>(obj);
		if (t != nullptr) {
			results.push_back(t);
		}
	}
	if (results.empty()) {
		// If nothing found, attempt matching by strings.
		// Why? Because dl and RTTI aren't playing nicely together;
		// each plugin might assign a different typeid to the same class.

		std::string classname = JTypeInfo::demangle<T>();
		for (auto obj : associated) {
			if (obj->className() == classname) {
				results.push_back(reinterpret_cast<const T*>(obj));
			}
		}
	}
	return results;
}

inline void JObject::Summarize(JObjectSummary& summary) const
{
    /// A virtual method which allows the user to display the contents of
    /// a JObject in a formatted, structured way. This is optional, but recommended.

    /// Fill in the JObjectSummary container with information for each
    /// member variable of the JObject using JObjectSummary::add(...).
    /// This accepts a reference to the variable itself, the variable name,
    /// a printf-style format string, and an optional description string which is
    /// good for communicating things like units.

    /// For convenience, we provide a NAME_OF macro, which will turn the member
    /// variable name into a string, rather than having the user write it out manually.
    /// This is useful if you use automatic refactoring tools, which otherwise
    /// might allow the variable name and the stringified name to get out of sync.

	summary.add((unsigned long) this, "JObject", "0x%08x");
}



#endif // _JObject_h_

