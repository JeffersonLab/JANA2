#ifndef _JFunctions_h_
#define _JFunctions_h_

#include <memory>
#include <string>
#include <cxxabi.h>

class JTaskBase;
class JEvent;
class JApplication;

std::shared_ptr<JTaskBase> JMakeAnalyzeEventTask(std::shared_ptr<const JEvent>&& aEvent, JApplication* aApplication);

//-------------------------------------------
// GetDemangledName
//-------------------------------------------
template <typename T>
std::string GetDemangledName(void)
{
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

#endif // _JFunctions_h_
