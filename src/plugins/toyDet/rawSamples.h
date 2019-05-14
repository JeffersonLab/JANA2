//
//    File: rawSamples.h
// Created: Wed May  1 13:41:20 EDT 2019
// Creator: pooser (on Linux rudy.jlab.org 3.10.0-957.10.1.el7.x86_64 x86_64)
//

#ifndef _rawSamples_h_
#define _rawSamples_h_

#include <vector>

#include <JANA/JObject.h>

using namespace std;

//////////////////////////////////////////////////////////////////////////////////////////////////
/// Brief class description.
///
/// Detailed class description.
//////////////////////////////////////////////////////////////////////////////////////////////////

class rawSamples:public JObject{

 public:
  
  rawSamples();
  virtual ~rawSamples();
  
  rawSamples(int eventNum, int chanNum, vector <double> &tdcData, vector <double> &adcData)
    : eventNum(eventNum), chanNum(chanNum), tdcData(tdcData), adcData(adcData) {}
 
  int eventNum, chanNum;

  vector <double> tdcData, adcData;
		
 protected:
	
	
 private:

};

#endif // _rawSamples_h_

