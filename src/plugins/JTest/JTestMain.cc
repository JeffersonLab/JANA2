//
//    File: JTestMain.cc
// Created: Fri Dec  7 08:42:59 EST 2018
// Creator: davidl (on Linux jana2.jlab.org 3.10.0-957.el7.x86_64 x86_64)
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
// Description
//
// This is used as an overall orchestrator for the various testing modes of the JTest
// plugin. This allows it to do more advanced testing by setting the JTEST:MODE and
// other config. parameters. This could have all been embedded in the JEventProcessor
// class, but I decided to put it here to make things a little cleaner.
//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

#include <memory>

#include <JANA/JApplication.h>
#include <JANA/JEventSourceGeneratorT.h>
#include <JANA/JCsvWriter.h>

#include "JTestParser.h"
#include "JTestPlotter.h"
#include "JTestFactoryGenerator.h"


extern "C"{
void InitPlugin(JApplication *app){

	InitJANAPlugin(app);
    app->Add(new JTestParser("dummy_source", app));
    app->Add(new JTestPlotter(app));
    app->Add(new JTestFactoryGenerator());

    // Demonstrates attaching a CSV writer so we can view the results from any JFactory
    app->SetParameterValue<std::string>("csv:dest_dir", ".");
    app->Add(new JCsvWriter<JTestTrackData>());

    // Demonstrates sharing user-defined services with our components
	app->ProvideService(std::make_shared<JTestCalibrationService>());

}
} // "C"

