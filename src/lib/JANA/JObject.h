//
//    File: JObject.h
// Created: Sun Oct 15 21:30:42 CDT 2017
// Creator: davidl (on Darwin harriet.local 15.6.0 i386)
//
// ------ Last repository commit info -----
// [ Date ]
// [ Author ]
// [ Source ]
// [ Revision ]
//
//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - 
// Jefferson Science Associates LLC Copyright Notice:  
// Copyright 251 2014 Jefferson Science Associates LLC All Rights Reserved. Redistribution
// and use in source and binary forms, with or without modification, are permitted as a
// licensed user provided that the following conditions are met:  
// 1. Redistributions of source code must retain the above copyright notice, this
//    list of conditions and the following disclaimer. 
// 2. Redistributions in binary form must reproduce the above copyright notice, this
//    list of conditions and the following disclaimer in the documentation and/or other
//    materials provided with the distribution.  
// 3. The name of the author may not be used to endorse or promote products derived
//    from this software without specific prior written permission.  
// This material resulted from work developed under a United States Government Contract.
// The Government retains a paid-up, nonexclusive, irrevocable worldwide license in such
// copyrighted data to reproduce, distribute copies to the public, prepare derivative works,
// perform publicly and display publicly and to permit others to do so.   
// THIS SOFTWARE IS PROVIDED BY JEFFERSON SCIENCE ASSOCIATES LLC "AS IS" AND ANY EXPRESS
// OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
// MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL
// JEFFERSON SCIENCE ASSOCIATES, LLC OR THE U.S. GOVERNMENT BE LIABLE TO LICENSEE OR ANY
// THIRD PARTES FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
// DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
// OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
// LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
// OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
// POSSIBILITY OF SUCH DAMAGE.
//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - 
//
// Description:
//
//
//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - 
#ifndef _JObject_h_
#define _JObject_h_

#include <string>
#include <sstream>
#include <set>
#include <vector>

/// The JObject class is a base class for all data classes.
/// (See JFactory for algorithm classes.)

// The following is a legacy definition that is no longer needed in JANA2.
#define JOBJECT_PUBLIC(T) \
	static const char* static_className(void) {return #T;}

class JObject{
	public:
		JObject();
		virtual ~JObject();
	
		virtual const std::string& className(void) const { return mName; }
	
		// Associated objects
		inline void AddAssociatedObject(const JObject *obj);
		inline void AddAssociatedObjectAutoDelete(JObject *obj, bool auto_delete=true);
		inline void RemoveAssociatedObject(const JObject *obj);
		inline void ClearAssociatedObjects(void);
		inline bool IsAssociated(const JObject* locObject) const {return (associated.find(locObject) != associated.end());}

		// Convert to strings with pretty formatting for printing
		bool append_types = false;
		virtual void toStrings(std::vector< std::pair<std::string, std::string> > &items)const;
		template<typename T> void AddString(std::vector<std::pair<std::string, std::string> > &items, const char *name, const char *format, const T &val) const;

	protected:
		std::string mName;
		std::set<const JObject*> associated;
		std::set<JObject*> auto_delete;
	
	private:

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

//--------------------------
// toStrings
//--------------------------
inline void JObject::toStrings(std::vector<std::pair<std::string,std::string> > &items) const
{
	/// Fill the given "items" vector with items representing the (important)
	/// data members of this object. The structure of "items" is a vector
	/// of pairs. The "first" element of the pair is the name of the item
	/// as it should be displayed when dumping the item to the screen. For
	/// example, one may wish to include units using a string like "r (cm)".
	/// The "second" element of the pair is a formatted string containing the
	/// value as it should be displayed.
	///
	/// To facilitate this, the AddString() method exists which allows
	/// items to be added with the desired formatting using a single line.
	///
	/// This is a virtual method that is expected (but not required)
	/// to be implemented by all classes that inherit from JObject.

	AddString(items, "JObject", "0x%08x", (unsigned long)this);
}

//--------------------------
// AddString
//--------------------------
template<typename T>
void JObject::AddString(std::vector<std::pair<std::string,std::string> > &items, const char *name, const char *format, const T &val) const
{
	/// Write the given value (val) to a string using the sprintf style formatting
	/// string (format) and add it to the given vector (items) with the column
	/// name "name". This is intended for use in the toStrings() method of
	/// classes that inherit from JObject.
	///
	/// The append_type flag provides a facility for recording the data type
	/// and value with default formatting into items. This can be used
	/// by a generic convertor (not part of JANA) to auto-generate a
	/// representation of this object for use in some other persistence
	/// package (e.g. ROOT files).
	///
	/// If the append_types flag is set then the data type of "val" is
	/// automatically appended with a colon (:) separator to the
	/// name (first) part of the pair. In addition, "val" is converted
	/// using stringstream and appended as well, also with a colon (:)
	/// separator. For example, if the value of name passed in is "px"
	/// and T is of type double, then the first member of the pair
	/// appended to items will be something like "px:double:1.23784"
	/// which can be decifered later to get the name, type, and value
	/// of the data member.
	///
	/// By default, the append_types flag is not set and the name part
	/// of the pair is a straight copy of the name argument that is
	/// passed in.

	char str[256];
	sprintf(str, format, val);

	std::stringstream ss;
	ss<<name;
	if(append_types){
		if(typeid(T)==typeid(int)){
			ss<<":int:"<<val;
		}else if(typeid(T)==typeid(int32_t)){
			ss<<":int:"<<val;
		}else if(typeid(T)==typeid(unsigned int)){
			ss<<":uint:"<<val;
		}else if(typeid(T)==typeid(uint32_t)){
			ss<<":uint:"<<val;
		}else if(typeid(T)==typeid(long)){
			ss<<":long:"<<val;
		}else if(typeid(T)==typeid(int64_t)){
			ss<<":long:"<<val;
		}else if(typeid(T)==typeid(unsigned long)){
			ss<<":ulong:"<<val;
		}else if(typeid(T)==typeid(uint64_t)){
			ss<<":ulong:"<<val;
		}else if(typeid(T)==typeid(short)){
			ss<<":short:"<<val;
		}else if(typeid(T)==typeid(int16_t)){
			ss<<":short:"<<val;
		}else if(typeid(T)==typeid(unsigned short)){
			ss<<":ushort:"<<val;
		}else if(typeid(T)==typeid(uint16_t)){
			ss<<":ushort:"<<val;
		}else if(typeid(T)==typeid(float)){
			ss<<":float:"<<val;
		}else if(typeid(T)==typeid(double)){
			ss<<":double:"<<val;
		}else if(typeid(T)==typeid(std::string)){
			ss<<":string:"<<val;
		}else if(typeid(T)==typeid(const char*)){
			ss<<":string:"<<val;
		}else if(typeid(T)==typeid(char*)){
			ss<<":string:"<<val;
		}else{
			ss<<":unknown:"<<str;
		}
	}

	std::pair<std::string, std::string> item;
	item.first = ss.str();
	item.second = std::string(str);
	items.push_back(item);
}

#endif // _JObject_h_

