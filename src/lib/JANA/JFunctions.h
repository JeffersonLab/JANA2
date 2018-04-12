#ifndef _JFunctions_h_
#define _JFunctions_h_

#include <memory>

class JTaskBase;
class JEvent;
class JApplication;

std::shared_ptr<JTaskBase> JMakeAnalyzeEventTask(std::shared_ptr<const JEvent>&& aEvent, JApplication* aApplication);

#endif // _JFunctions_h_
