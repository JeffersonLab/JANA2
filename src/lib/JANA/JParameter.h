//
//    File: JParameter.h
// Created: Sat Oct 21 09:32:51 EDT 2017
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
#ifndef _JParameter_h_
#define _JParameter_h_

#include <string>
#include <sstream>
#include <vector>

class JParameter{
	public:
		template<typename T>
		JParameter(std::string name, T val);
		virtual ~JParameter();
		
		template<typename T>
		T GetDefault(void);
		
		bool GetHasDefault(void);
	
		const std::string& GetName(void);

		template<typename T>
		T GetValue(void);

		template<typename T>
		T GetValue(T &val);
		
		bool IsDefault(void);

		template<typename T>
		T SetDefault(T val);

		template<typename T>
		T SetValue(T val);
		
	protected:
		
		bool _has_default;
	
		std::string _name;
		std::string _val;
		std::string _default;
	
	private:

};

//---------------------------------
// JParameter    (Constructor)
//---------------------------------
template<typename T>
JParameter::JParameter(std::string name, T val):_has_default(false),_name(name),_default("")
{
	SetValue<T>(val);
}

//---------------------------------
// GetDefault
//---------------------------------
template<typename T>
inline T JParameter::GetDefault(void)
{
	std::stringstream ss(_default);
	T val;
	ss >> val;
	
	return val;
}

template<>
inline std::vector<std::string> JParameter::GetDefault()
{
    std::stringstream ss(_default);
    std::vector<std::string> result;
    std::string s;
    while (getline(ss, s, ',')) {
        result.push_back(s);
    }
    return result;
}


//---------------------------------
// GetValue
//---------------------------------
template<typename T>
inline T JParameter::GetValue(void)
{
	std::stringstream ss(_val);
	T val;
	ss >> val;
	
	return val;
}


/// Template specialization for parsing as vector of size_t
template<>
inline std::vector<size_t> JParameter::GetValue<std::vector<size_t>>() {

    std::stringstream ss(_val);
    std::vector<size_t> result;
    std::string s;
    while (getline(ss, s, ',')) {
        std::stringstream sss(s);
        size_t x;
        sss >> x;
        result.push_back(x);
    }
    return result;
}

/// Template specialization for parsing as vector of string
template<>
inline std::vector<std::string> JParameter::GetValue<std::vector<std::string>>() {

    std::stringstream ss(_val);
    std::vector<std::string> result;
    std::string s;
    while (getline(ss, s, ',')) {
        result.push_back(s);
    }
    return result;
}


//---------------------------------
// GetValue
//---------------------------------
template<typename T>
inline T JParameter::GetValue(T &val)
{
	val = GetValue<T>();
	return val;
}

//---------------------------------
// SetDefault
//---------------------------------
template<typename T>
inline T JParameter::SetDefault(T def)
{
	T old_def = GetDefault<T>();

	std::stringstream ss;
	ss << def;
	_default = ss.str();

	_has_default = true;

	return old_def;
}


/// Specialization for vector<string>
template<>
inline std::vector<std::string> JParameter::SetDefault(std::vector<std::string> default_value)
{
    auto old_def = GetDefault<std::vector<std::string>>();

    std::stringstream ss;
    size_t len = default_value.size();
    for (size_t i=0; i+1<len; ++i) {
        ss << default_value[i];
        ss << ",";
    }
    if (len != 0) {
        ss << default_value[len-1];
    }

    _default = ss.str();
    _has_default = true;

    return old_def;
}


//---------------------------------
// SetValue
//---------------------------------
template<typename T>
inline T JParameter::SetValue(T val)
{
	T old_val = GetValue<T>();

	std::stringstream ss;
	ss << val;
	_val = ss.str();

	return old_val;
}

template<>
inline std::vector<std::string> JParameter::SetValue(std::vector<std::string> val)
{
    auto old_val = GetValue<std::vector<std::string>>();

    std::stringstream ss;
    size_t len = val.size();
    for (size_t i=0; i+1<len; ++i) {
        ss << val[i];
        ss << ",";
    }
    if (len > 0) {
        ss << val[len-1];
    }
    _val = ss.str();

    return old_val;
}


#endif // _JParameter_h_

