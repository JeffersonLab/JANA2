//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// Jefferson Science Associates LLC Copyright Notice:
//
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
// Author: Nathan Brei
//

#ifndef JANA1_JERROR_H
#define JANA1_JERROR_H

/// This file contains error codes for errors specific to the
/// analysis code. Many functions return values
/// of type jerror_t. This header should be included in all
/// files which must deal with this type.

#define _DBG_ std::cerr<<__FILE__<<":"<<__LINE__<<" "
#define _DBG__ std::cerr<<__FILE__<<":"<<__LINE__<<std::endl

enum jerror_t{
	NOERROR = 0,
	UNKNOWN_ERROR = -1000,

	MAX_EVENT_PROCESSORS_EXCEEDED,

	ERROR_OPENING_EVENT_SOURCE,
	ERROR_CLOSING_EVENT_SOURCE,
	NO_MORE_EVENTS_IN_SOURCE,
	NO_MORE_EVENT_SOURCES,
	EVENT_NOT_IN_MEMORY,
	EVENT_SOURCE_NOT_OPEN,
	OBJECT_NOT_AVAILABLE,
	DEVENT_OBJECT_DOES_NOT_EXIST,

	MEMORY_ALLOCATION_ERROR,

	RESOURCE_UNAVAILABLE,
	VALUE_OUT_OF_RANGE,

	INFINITE_RECURSION,
	UNRECOVERABLE_ERROR,

	FILTER_EVENT_OUT
};



#endif //JANA2_JERROR_H
