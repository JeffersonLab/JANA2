//*************************************************************
// JStreamLog.h - Header file for stream-based logging.
// Author:	Craig Bookwalter
// Date:	Aug 2005
// Notes: 	Much of this was derived from examples given by
// 	D. Kuehl at http://www.inf.uni-konstanz.de/~kuehl/iostream/
// 	Also, many thanks to J. Hardie for his assistance with 
//  obscure protected-member rules. 
//*************************************************************


#ifndef _DSTREAMLOG_H_
#define _DSTREAMLOG_H_

#include <iostream>
#include <fstream>
#include <string>
#include "JStreamLogBuffer.h"

/// JStreamLog provides an intuitive interface for logging errors
/// and messages. To use:
///	
///
/// #include <iostream>
/// #include <fstream>	
///	#include "JStreamLog.h" 
///
/// using namespace std;
///
/// int main() {
///		JStreamLog info(cout, "INFO");
///			
///		info << "Some information. " << endMsg;
///		
///		ofstream f("errors.log");		
///		JStreamLog err(f, "ERR");
///		
///		err << "There was " << endl << "an error." << endMsg;
/// 	return 0;
/// }
///
///	Some details:
///  - The instantiation of a JStreamLog requires that an output
/// stream and a tag be provided. To write a single message to
/// a JLogStream object, be sure to pass in endMsg to make sure
/// the buffer is flushed.
///  - The endMsg manipulator uses a sentinel value from the ASCII table,
/// namely the ACK character, because it's not often used. If it should
/// somehow sneak into your message, it will split your message at the point
/// where ACK entered the stream.
///

class JStreamLog : public std::ostream
{
	public:
		JStreamLog(const std::ostream& os=std::cout, const char* tag="INFO");
		JStreamLog(const std::ostream& os, const std::string& tag);
		JStreamLog(std::streambuf* buf, const char* tag);
		virtual ~JStreamLog();

};

std::ostream& endMsg(std::ostream& os);
#endif //_DSTREAMLOG_H_
