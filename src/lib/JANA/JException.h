//
//    File: JException.h
// Created: Fri Oct 20 09:36:30 EDT 2017
// Creator: davidl (on Darwin harriet.jlab.org 15.6.0 i386)
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
#ifndef _JException_h_
#define _JException_h_

#include <string>

struct JException {
public:
    JException(std::string mess = "");

    virtual ~JException();

    template<typename T>
    JException(std::string mess, T t);

    template<typename T, typename U>
    JException(std::string mess, T t, U u);

    template<typename T, typename U, typename V>
    JException(std::string mess, T t, U u, V v);

    // Deprecated
    std::string GetMessage();

    friend std::ostream& operator<<(std::ostream& os, JException const& ex);

    /// JException is a data object which attaches JANA-specific context information to a generic exception.
    /// As it unwinds the call stack, different exception handlers may add or change information as they see fit.
    /// It does not use getters and setters, because it does not need these, because there is no invariant.
    std::string message;
    std::string plugin_name;
    std::string component_name;
    std::string factory_name;
    std::string factory_tag;
    std::string backtrace;

};

//---------------------------------
// JException    (Constructor)
//---------------------------------
template<typename T>
JException::JException(std::string mess, T t) {
    char cmess[1024];
    snprintf(cmess, 1024, mess.c_str(), t);
    message = cmess;
}

//---------------------------------
// JException    (Constructor)
//---------------------------------
template<typename T, typename U>
JException::JException(std::string mess, T t, U u) {
    char cmess[1024];
    snprintf(cmess, 1024, mess.c_str(), t, u);
    message = cmess;
}

//---------------------------------
// JException    (Constructor)
//---------------------------------
template<typename T, typename U, typename V>
JException::JException(std::string mess, T t, U u, V v) {
    char cmess[1024];
    snprintf(cmess, 1024, mess.c_str(), t, u, v);
    message = cmess;
}

#endif // _JException_h_

