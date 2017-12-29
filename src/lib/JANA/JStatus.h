//
//    File: JStatus.h
// Created: Wed Dec 27 18:08:16 EST 2017
// Creator: davidl (on Darwin harriet 15.6.0 i386)
//
// ------ Last repository commit info -----
// [ Date ]
// [ Author ]
// [ Source ]
// [ Revision ]
//
//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - 
// Jefferson Science Associates LLC Copyright Notice:  
// Copyright 251 2014 Jefferson Science Associates LLC All Rights Reserved. Redistribution
// and use in source and binary forms, with or without modification, are permitted as a
// licensed user provided that the following conditions are met:  
// 1. Redistributions of source code must retain the above copyright notice, this
//    list of conditions and the following disclaimer. 
// 2. Redistributions in binary form must reproduce the above copyright notice, this
//    list of conditions and the following disclaimer in the documentation and/or other
//    materials provided with the distribution.  
// 3. The name of the author may not be used to endorse or promote products derived
//    from this software without specific prior written permission.  
// This material resulted from work developed under a United States Government Contract.
// The Government retains a paid-up, nonexclusive, irrevocable worldwide license in such
// copyrighted data to reproduce, distribute copies to the public, prepare derivative works,
// perform publicly and display publicly and to permit others to do so.   
// THIS SOFTWARE IS PROVIDED BY JEFFERSON SCIENCE ASSOCIATES LLC "AS IS" AND ANY EXPRESS
// OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
// MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL
// JEFFERSON SCIENCE ASSOCIATES, LLC OR THE U.S. GOVERNMENT BE LIABLE TO LICENSEE OR ANY
// THIRD PARTES FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
// DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
// OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
// LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
// OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
// POSSIBILITY OF SUCH DAMAGE.
//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - 
//
// Description:
//
//
//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - 
#ifndef _JStatus_h_
#define _JStatus_h_

#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include <string>
#include <sstream>

#include <JANA/JApplication.h>

class JStatus;
extern JStatus *gJSTATUS;

class JStatus{
	public:

		//---------------------------------
		// Report
		//---------------------------------
		static void Report(void){
		
			/// Static method used to generate and write the current status of the JANA process
			/// to the named pipe. On the first call, this will create a JStatus object and
			/// create the named pipe (see JANA::STATUS_FNAME for name). Subsequent calls
			/// will open the pipe, write to it, and close it. This is generally called when
			/// the process receives a USR1 signal. Typically, if a single JANA process is
			/// running on the node, then one can issue a "killall <progname> -USR1" and then
			/// do a "cat /tmp/jana_status" to see the report.
		
			// Create a JStatus object if one does not already exist
			if( gJSTATUS == nullptr ) new JStatus();

			// Generate a report
			std::stringstream ss;
			gJSTATUS->GenerateReport( ss );
			gJSTATUS->SendReport( ss );

		}
		
		static void RecordBackTrace(void);
		
		void GenerateReport(std::stringstream &ss);
		void SendReport(std::stringstream &ss);
		
	protected:
		
		JStatus();
		virtual ~JStatus();

		std::string path;

};


#endif // _JStatus_h_

