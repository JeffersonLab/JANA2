// $Id$
//
//    File: JThread.h
// Created: Wed Dec 11 15:44:13 EST 2013
// Creator: hdsys (on Linux gluon04.jlab.org 2.6.32-358.14.1.el6.x86_64 x86_64)
//

#ifndef _JThread_
#define _JThread_

#include <JANA/jerror.h>

#include <pthread.h>

// Place everything in JANA namespace
namespace jana{

class JEventLoop;

class JThread{
	public:
		JThread(JEventLoop *loop=NULL):loop(loop),heartbeat(0.0),thread_id(pthread_self()),printed_stall_warning(false){}
		virtual ~JThread(){}
		
		JEventLoop *loop;
		double heartbeat;
		pthread_t thread_id;
		bool printed_stall_warning;

};


} // Close JANA namespace


#endif // _JThread_

