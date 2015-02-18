// $Id$
//
//    File: janactl_plugin.h
// Created: Tue Dec 22 07:03:32 EST 2009
// Creator: davidl (on Darwin harriet.jlab.org 9.8.0 i386)
//
 	


#ifndef _janactl_plugin_
#define _janactl_plugin_

#include <vector>

#include <JANA/JApplication.h>
#include <JANA/JStreamLog.h>
using namespace jana;

#if HAVE_CMSG

#include <cMsg.hxx>
using namespace cmsg;


class janactl_plugin:public cMsgCallback{
	public:
		janactl_plugin(JApplication *japp);
		virtual ~janactl_plugin();

	protected:

		void callback(cMsgMessage *msg, void *userObject);

		void HostStatusSYSCTL(vector<string> &keys, vector<string> &vals);
		void HostStatusPROC(vector<string> &keys, vector<string> &vals);

		void HostInfoSYSCTL(vector<string> &keys, vector<string> &vals);
		void HostInfoPROC(vector<string> &keys, vector<string> &vals);

	private:
		janactl_plugin(void);

		cMsg *cMsgSys;		
		string myname;
		JApplication *japp;
		JStreamLog jctlout;
		int VERBOSE;
		
		std::vector<void*> subscription_handles;
};

#endif // HAVE_CMSG


#endif // _janactl_plugin_

