//****************************************************************
// JLog.h - Error-logging class for use with the Hall D analysis 
// framework.
// 
// At some point, there will be a global log that all subsystems will
// log errors to. For now, in your own subsystem, create yourself a 
// JLog object in one of the following ways:
//
//	JLog* log = new JLog();    // Only display "ERROR"-level messages (on cerr)
//  
//
//	ofstream info ("info.log");
//  ofstream warn ("warn.log");
//	JLog* log = new JLog(info, warn, cerr);  // Put "INFO"-, "WARN"-, and 
//											 // "ERR"-level messages into these
//											 // respective streams.
//
// After that, to log a message to your JLog, use:
// 
//	log("A blankity-blank error occurred.", ERR);
//  
//
// 
// Author: Craig Bookwalter
// July 2005
//
// TODO: Add an ID string to identify who the log belongs to?
// TODO: use DOxygen to do the docs.
// TODO: operator()?
//****************************************************************
#ifndef _DLOG_H_
#define _DLOG_H_

#include <stdexcept>
#include <iostream>
#include <fstream>
#include <string>
#include <string.h>

enum {
	INFO,
	WARN,
	ERR
};

class JLog
{
	public:
		JLog();
		JLog(std::ostream& info, 
			 std::ostream& warn, 
			 std::ostream& err,
			 int level = 0);
		virtual ~JLog();
		void setLoggingLevel(int level);		
		inline void log(std::string msg, int level) {
			if (level < __level)
				return;
			time_t ts = time(0);
			char* t = ctime(&ts); 
			t[strlen(t)-1] = 0;
			switch (level) {
				case INFO : 
					*__info << "<INFO " 
							<< t 
							<< "> "
							<< msg 
							<< std::endl;
				case WARN :
					*__warn << "<WARNING " 
							<< t
							<< "> "
							<< msg
							<< std::endl;
				case ERR  :
					*__err  << "<ERROR "
							<< t
							<< "> "
							<< msg
							<< std::endl;
			}
		}
		inline void log(char* msg, int level){
			log(std::string(msg), level);
		}
		inline void operator() (std::string msg, int level){
			log(msg, level);
		}
		inline void operator() (char* msg, int level) {
			log(msg, level);
		}
			
	private:
		std::ostream* __info;
		std::ostream* __warn;
		std::ostream* __err;
		int __level;
};

// Global variable used for logging
extern JLog dlog;

#endif /*_DLOG_H_*/
