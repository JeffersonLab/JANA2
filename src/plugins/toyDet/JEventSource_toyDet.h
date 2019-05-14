//
//    File: toyDet/JEventSource_toyDet.h
// Created: Wed Apr 24 16:04:14 EDT 2019
// Creator: pooser (on Linux rudy.jlab.org 3.10.0-957.10.1.el7.x86_64 x86_64)
//
// ------ Last repository commit info -----
// [ Date ]
// [ Author ]
// [ Source ]
// [ Revision ]

#ifndef _JEventSource_toyDet_h_
#define _JEventSource_toyDet_h_

#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <mutex>

#include <JANA/JEventSource.h>
#include <JANA/JEventSourceGeneratorT.h>

#include "JEvent_toyDet.h"

#include "rawSamples.h"

using namespace std;

//////////////////////////////////////////////////////////////////////////////////////////////////
/// Brief class description.
///
/// Detailed class description.
//////////////////////////////////////////////////////////////////////////////////////////////////

class JEventSource_toyDet : public JEventSource{
  
 public:
                
  JEventSource_toyDet(std::string source_name, JApplication *app);
  virtual ~JEventSource_toyDet();
		
  static std::string GetDescription(void){ return "My Event source"; }
  void Open(void);
  void GetEvent(std::shared_ptr<JEvent>);
  bool GetObjects(const std::shared_ptr<const JEvent>& aEvent, JFactory* aFactory);

 private:
  
  // User defined variables
  ifstream ifs;
  string   line;

  vector <double> tdcData, adcData;
  vector <rawSamples*> chanData;
  vector < vector<rawSamples*> > eventData;

};

#endif // _JEventSource_toyDet_h_

