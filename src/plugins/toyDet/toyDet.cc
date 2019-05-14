//
//    File: toyDet/toyDet.cc
// Created: Wed Apr 24 16:04:14 EDT 2019
// Creator: pooser (on Linux rudy.jlab.org 3.10.0-957.10.1.el7.x86_64 x86_64)
//
// ------ Last repository commit info -----
// [ Date ]
// [ Author ]
// [ Source ]
// [ Revision ]


#include "JEventProcessor_toyDet.h"
#include "JEventSource_toyDet.h"
#include "JFactoryGenerator_toyDet.h"

extern "C"{
  void InitPlugin(JApplication *app){
    InitJANAPlugin(app);

    app->Add(new JEventProcessor_toyDet());
    app->Add(new JEventSourceGeneratorT<JEventSource_toyDet>());
    app->Add(new JFactoryGenerator_toyDet());

  }
} // "C"
