// $Id$
//
//    File: JEventProcessor_jana_test.h
// Created: Mon Oct 23 22:38:48 EDT 2017
// Creator: davidl (on Darwin harriet 15.6.0 i386)
//

#ifndef _JEventProcessor_jana_test_
#define _JEventProcessor_jana_test_

#include <atomic>

#include <JANA/JEventProcessor.h>
#include <JANA/JEvent.h>

class JTestEventProcessor:public JEventProcessor{
	public:
		JTestEventProcessor(JApplication*);
		~JTestEventProcessor();
		const char* className(void){return "JEventProcessor_jana_test";}

		virtual void Init(void);
		virtual void Process(const std::shared_ptr<const JEvent>& aEvent);
		virtual void Finish(void);

	private:
		std::atomic<std::size_t> mNumObjects{0};
};

#endif // _JEventProcessor_jana_test_

