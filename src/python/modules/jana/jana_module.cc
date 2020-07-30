//
//    File: janapy/janapy.cc
// Created: Fri Dec 21 23:05:11 EST 2018
// Creator: davidl (on Darwin amelia.jlab.org 17.7.0 i386)
//
// ------ Last repository commit info -----
// [ Date: Tue Feb 26 18:13:39 2019 -0500 ]
// [ Author: Nathan Brei <nbrei@halld3.jlab.org> ]
// [ Source: src/plugins/janapy/janapy.cc ]
// [ Revision: c15aad0b0dec2e6f8f29d1f727b2daec6c7cf376 ]
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


#include <janapy.h>


//void JANA_PythonModuleInit(JApplication *sApp);

//static bool PY_INITIALIZED = false; // See JANA_PythonModuleInit


// This is temporary and will likely be changed once the new arrow
// system is fully adopted.
//static JApplication *pyjapp = nullptr;

//#ifndef _DBG__
//#define _DBG__ std::cout<<__FILE__<<":"<<__LINE__<<std::endl
//#define _DBG_ std::cout<<__FILE__<<":"<<__LINE__<<" "
//#endif



////================================================================================
//
////-------------------------------------
//// JANA_PythonModuleInit
////-------------------------------------
//void JANA_PythonModuleInit(JApplication *sApp)
//{
//    /// This will initialize the embedded Python interpreter and import the
//    /// JANA module (defined by the routines in this file). This gets called
//    /// when the janapy plugin gets initialized. It will then automatically
//    /// look for a file whose name is given by either the JANA_PYTHON_FILE
//    /// config. parameter or environment variable in that order. If neither
//    /// exists, it looks for a file name "jana.py" in the current working
//    /// directory and if found, executes that. If you do not wish to execute
//    /// a python file, then you can set the JANA_PYTHON_FILE value to an empty
//    /// string (or better yet, just not attach the janapy plugin!).
//    ///
//    /// Note that janapy creates a dedicated thread that the python interpreter
//    /// runs in.
//    ///
//    /// IMPORTANT: The janapy InitPlugin routine will block (resulting in the whole
//    /// JANA system pausing) until either this routine finishes or the python
//    /// script it invokes calls "jana.Start()". This is to give the python script
//    /// a chance to modify running conditions prior to event processing starting.
//    /// If the python script intends to run throughout the life of the process,
//    /// then it MUST call jana.Start() at some point. If the script is small and only
//    /// runs for a short time, then you don't need to call it since it will be
//    /// called automatically when the script ends.
//
//    // Start the interpreter and keep it alive.
//    // NOTE: the interpreter will stop and be deleted once we leave this routine.
//    // This only happens when the python script returns so there will no longer
//    // be any need for it.
//    py::scoped_interpreter guard{};
//
//    // Use existing JApplication.
//    pyjapp = sApp;
//
//    // Get name of python file to execute
//    string fname = "jana.py";
//    try{
//        fname = pyjapp->GetParameterValue<string>("JANA_PYTHON_FILE");
//    }catch(...){
//        auto JANA_PYTHON_FILE = getenv("JANA_PYTHON_FILE");
//        if( JANA_PYTHON_FILE ) fname = JANA_PYTHON_FILE;
//    }
//
//    // Execute python script.
//    // n.b. The script may choose to run for the lifetime of the program!
//    py::eval_file(fname);
//
//    PY_INITIALIZED = true;
//}


//================================================================================
// Module definition
// The arguments of this structure tell Python what to call the extension,
// what it's methods are and where to look for it's method definitions.
// The routines themselves are all defined in src/python/common/janapy.h
// and src/python/common/janapy.cc.
PYBIND11_MODULE(jana, m) {

    m.doc() = "JANA2 Python Interface";

    // (see src/python/common/janapy.h)
    JANA_MODULE_DEF

    pyjapp = new JApplication();
}
