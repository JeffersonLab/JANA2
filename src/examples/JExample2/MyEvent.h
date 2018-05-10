

#ifndef _MYHIT_H_
#define _MYHIT_H_

#include "MyHit.h"

class MyEvent : public JEvent{
	public:
		// Nothing here for this simple example. For a real implementation
		// though we would keep a pointer to a buffer holding the event.
		// This would then be available when the GetObjects method is called.
};

#endif // _MYHIT_H_
