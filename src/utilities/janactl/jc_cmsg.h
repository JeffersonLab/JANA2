// $Id$
//
//    File: jc_cmsg.h
// Created: Sun Dec 27 23:31:21 EST 2009
// Creator: davidl (on Darwin harriet.jlab.org 9.8.0 i386)
//

#ifndef _jc_cmsg_
#define _jc_cmsg_

#include <sys/time.h>

#include <vector>

#if HAVE_CMSG
#include <cMsg.hxx>
using namespace cmsg;


class jc_cmsg:public cMsgCallback{
	public:
		jc_cmsg(string myUDL, string myName, string myDescr);
		virtual ~jc_cmsg();
		
		class thrinfo_t{
			public:
				uint64_t thread;
				uint64_t Nevents;
				double rate_instantaneous;
				double rate_average;
		};
		
		bool IsConnected(void){return is_connected;}
		static double GetTime(void){struct itimerval tmr; getitimer(ITIMER_REAL, &tmr); return TimerToDouble(tmr);}
		static double TimerToDouble(struct itimerval &tmr){return (double)tmr.it_value.tv_sec + (double)(tmr.it_value.tv_usec)*1.0E-6;}
		double GetTimeout(void){return timeout;}
		void SetTimeout(double timeout){this->timeout = timeout;}
		void SendCommand(string cmd, string subject="janactl");
		void PingServers(void);
		
		void ListRemoteProcesses(void);
		void GetThreadInfo(string subject);
		void ListConfigurationParameters(string subject);
		void ListSources(string subject);
		void ListSourceTypes(string subject);
		void ListFactories(string subject);
		void ListPlugins(string subject);
		void GetCommandLine(string subject);
		void GetHostInfo(string subject);
		void AttachPlugin(string subject, string plugin);

		pthread_mutex_t mutex;
		double start_time;
		double last_ping_time;
		double last_threadinfo_time;
		map<string, double> last_msg_received_time;
		map<string, vector<thrinfo_t> > thrinfos;
		map<string, string> config_params;
		string config_params_responder; // only print results from first to respond
		map<string, vector<pair<string, string> > > sources;
		map<string, vector<pair<string, string> > > hostInfos;
		vector<pair<string, string> > commandLines;

	protected:

		void callback(cMsgMessage *msg, void *userObject);

	private:
		jc_cmsg(void);

		cMsg *cMsgSys;
		string myname;
		bool is_connected;
		std::vector<void*> subscription_handles;
		double timeout;
};

#endif // HAVE_CMSG

#endif // _jc_cmsg_

