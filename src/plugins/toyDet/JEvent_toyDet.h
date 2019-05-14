//
//    File: toyDet/JEvent_toyDet.h
// Created: Wed Apr 24 16:04:14 EDT 2019
// Creator: pooser (on Linux rudy.jlab.org 3.10.0-957.10.1.el7.x86_64 x86_64)
//
// ------ Last repository commit info -----
// [ Date ]
// [ Author ]
// [ Source ]
// [ Revision ]


#ifndef _JEvent_toyDet_h_
#define _JEvent_toyDet_h_

#include <JANA/JEvent.h>

//////////////////////////////////////////////////////////////////////////////////////////////////
/// Brief class description.
///
/// Detailed class description.
//////////////////////////////////////////////////////////////////////////////////////////////////
class JEvent_toyDet : public JEvent{
 public:
		
  JEvent_toyDet() {}
  virtual ~JEvent_toyDet() {}

  // This represents an event read from a JEventSource_toyDet object
  // Add members that can hold the data in whatever form is easy to access
  // in the JEventSource_toyDet::GetObjects method
};

#endif // _JEvent_toyDet_h_

