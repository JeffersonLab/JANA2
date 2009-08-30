// $Id$
//
//    File: rs_cmsg.h
// Created: Thu Aug 28 20:00 EDT 2009
// Creator: davidl (on Darwin harriet.jlab.org 9.8.0 i386)
//

#ifndef _rs_cmsg_
#define _rs_cmsg_

#include <vector>


#include <cMsg.hxx>
using namespace cmsg;

class rs_mainframe;

class rs_cmsg:public cMsgCallback{
	public:
		rs_cmsg(void);
		virtual ~rs_cmsg();
		
		void PingServers(void);
		void RequestHists(string servername);
		void RequestHistogram(string servername, string hnamepath);
		
	protected:

		void callback(cMsgMessage *msg, void *userObject);
	
	private:
		cMsg *cMsgSys;
		
		string myname;
		
		std::vector<void*> subscription_handles;
};

#endif // _rs_cmsg_

