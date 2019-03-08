#pragma once

#include <cxxabi.h>

namespace JTypeInfo {

template <typename T>
std::string demangle(void) {

	/// Return the demangled name (if available) for the type the template
	/// is based. Call it like this:
	///   cout << GetDemangledName<MyType>() << endl;
	int status=-1;
	auto cstr = abi::__cxa_demangle(typeid(T).name(), NULL, NULL, &status);
	std::string type(cstr);
	free(cstr);
	if( status != 0 ) type = typeid(T).name();
	return type;
}

}


