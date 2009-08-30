// $Id$
//
//    File: DRootSpy.h
// Created: Thu Aug 27 13:40:02 EDT 2009
// Creator: davidl (on Darwin harriet.jlab.org 9.8.0 i386)
//

#ifndef _DRootSpy_
#define _DRootSpy_

#include <vector>


#include <cMsg.hxx>

using namespace cmsg;


class DRootSpy:public cMsgCallback{
	public:
		DRootSpy(void);
		virtual ~DRootSpy();
		
		class hinfo_t{
			public:
				string name;
				string title;
				string type;
				string path;
		};
		
	protected:

		void callback(cMsgMessage *msg, void *userObject);
		void AddRootObjectsToList(TDirectory *dir, vector<hinfo_t> &hinfos);
	
	private:
		cMsg *cMsgSys;
		
		string myname;
		
		std::vector<void*> subscription_handles;
};

#endif // _DRootSpy_

