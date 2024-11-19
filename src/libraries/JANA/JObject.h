
// Copyright 2020, Jefferson Science Associates, LLC.
// Subject to the terms in the LICENSE file found in the top-level directory.

#pragma once

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

    /// add() is used to insert a new row of JObjectMember data
    void add(const JObjectMember &objectMember){
        m_fields.push_back(objectMember);
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

        // JANA1 compatibility getters
        template<typename T>
        void GetSingle(const T* &ptrs, std::string classname="") const;

        template<typename T>
        void GetT(std::vector<const T*> &ptrs) const;

        template<typename T>
        void Get(std::vector<const T*> &ptrs, std::string classname="", int max_depth=1000000) const;

        template<typename T>
        void GetAssociatedAncestors(std::set<const JObject*> &already_checked,
                                    int &max_depth,
                                    std::set<const T*> &objs_found,
                                    std::string classname="") const;



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
    // for (auto obj : associated) {
    //     const T* t = dynamic_cast<const T*>(obj);
    //     if (t != nullptr) {
    //         results.push_back(t);
    //     }
    // }
    // if (results.empty()) {
    //     // If nothing found, attempt matching by strings.
    //     // Why? Because dl and RTTI aren't playing nicely together;
    //     // each plugin might assign a different typeid to the same class.

    //     std::string classname = JTypeInfo::demangle<T>();
    //     for (auto obj : associated) {
    //         if (obj->className() == classname) {
    //             results.push_back(reinterpret_cast<const T*>(obj));
    //         }
    //     }
    // }
    Get(results);
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


/// The following have been added purely for compatibility with JANA1, in order to make
/// porting halld_recon more tractable.

template<class T>
void JObject::GetSingle(const T* &t, std::string classname) const
{
    /// This is a convenience method that can be used to get a pointer to the single
    /// associate object of type T.
    ///
    /// The objects are chosen by matching their class names
    /// (obtained via JObject::className()) either
    /// to the one provided in classname or to T::static_className()
    /// if classname is an empty string.
    ///
    /// If no object of the specified type is found, a NULL pointer is
    /// returned.

    t = NULL;

    if(classname=="")classname=T::static_className();

    //map<const JObject*, string>::const_iterator iter = associated.begin();
    //for(; iter!=associated.end(); iter++){
    for( auto obj : associated ){
        if( classname == obj->className() ){
            t = dynamic_cast<const T*>(obj);
            if(t!=NULL)return;
        }
    }
}

template<typename T>
void JObject::GetT(std::vector<const T*> &ptrs) const
{
    /// Fill the given vector with pointers to the associated
    /// JObjects of the type on which the vector is based. This is
    /// similar to the Get() method except objects are selected
    /// by attempting a dynamic_cast to type const T*. This allows
    /// one to select a list of all objects who have a type T
    /// somewhere in their inheritance chain.
    ///
    /// A potential issue with this method is that the dynamic_cast
    /// does not always work correctly for objects created via a
    /// plugin when the cast occurs outside of the plugin or
    /// vice versa.
    ///
    /// The contents of ptrs are cleared upon entry.

    ptrs.clear();

    //map<const JObject*, string>::const_iterator iter = associated.begin();
    //for(; iter!=associated.end(); iter++){
    for( auto obj : associated ){
        const T *ptr = dynamic_cast<const T*>(obj);
        if(ptr != NULL)ptrs.push_back(ptr);
    }
}


template<typename T>
void JObject::Get(std::vector<const T*> &ptrs, std::string classname, int max_depth) const
{
    /// Fill the given vector with pointers to the associated objects of the
    /// type on which the vector is based. The objects are chosen by matching
    /// their class names (obtained via JObject::className()) either to the
    /// one provided in classname or to T::static_className() if classname is
    /// an empty string. Associations will be searched to a level of max_depth
    /// to find all objects of the requested type. By default, max_depth is
    /// set to a very large number so that all associations are found. To
    /// limit the search to only objects directly associated with this one,
    /// set max_depth to either "0" or "1".
    ///
    /// The contents of ptrs are cleared upon entry.

    if(classname=="")classname=T::static_className();

    // Use the GetAssociatedAncestors method which may call itself
    // recursively to search all levels of association (at or
    // below this object. Objects for which this is an associated
    // object are not checked for).
    std::set<const JObject*> already_checked;
    std::set<const T*> objs_found;
    int my_max_depth = max_depth;
    GetAssociatedAncestors(already_checked, my_max_depth, objs_found, classname);

    // Copy results into caller's container
    ptrs.clear();
    ptrs.insert(ptrs.end(), objs_found.begin(), objs_found.end());
//	set<const T*>::iterator it;
//	for(it=objs_found.begin(); it!=objs_found.end(); it++){
//		ptrs.push_back(*it);
//	}
}

template<typename T>
void JObject::GetAssociatedAncestors(std::set<const JObject*> &already_checked, int &max_depth, std::set<const T*> &objs_found, std::string classname) const
{
    /// Get associated objects of the specified type (either "T" or classname).
    /// Check also for associated objects of any associated objects
    /// to a level of max_depth associations. This method calls itself
    /// recursively so care is taken to only check the associated objects
    /// of each object encountered only once.
    ///
    /// The "already_checked" parameter should be passed in as an empty container
    /// that is used to keep track of which objects had their direct associations
    /// checked. "max_depth" indicates the maximum level of associations to check
    /// (n.b. both "0" and "1" means only check direct associations.) This must
    /// be passed as a reference to an existing int since it is modified in order
    /// to keep track of the current depth in the recursive calls. Set max_depth
    /// to a very high number (like 1000000) to check all associations. The
    /// "objs_found" container will contain the actual associated objects found.
    /// The objects are chosen by matching their class names (obtained via
    /// JObject::className()) either to the one provided in "classname" or to
    /// T::static_className() if classname is an empty string.

    if(already_checked.find(this) == already_checked.end()) already_checked.insert(this);

    if(classname=="")classname=T::static_className();
    max_depth--;

    //map<const JObject*, string>::const_iterator iter = associated.begin();
    for( auto obj : associated ){

        // Add to list if appropriate
        if( classname == obj->className() ){
            objs_found.insert( dynamic_cast<const T*>(obj) );
        }

        // Check this object's associated objects if appropriate
        if(max_depth<=0) continue;
        if(already_checked.find(obj) != already_checked.end()) continue;
        already_checked.insert(obj);
        obj->GetAssociatedAncestors(already_checked, max_depth, objs_found, classname);
    }

    max_depth++;
}


