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
	std::string type = abi::__cxa_demangle(typeid(T).name(), NULL, NULL, &status);
	return status==0 ? type:std::string(typeid(T).name());
}

#endif // _JFunctions_h_
