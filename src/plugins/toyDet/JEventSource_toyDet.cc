//
//    File: toyDet/JEventSource_toyDet.cc
// Created: Wed Apr 24 16:04:14 EDT 2019
// Creator: pooser (on Linux rudy.jlab.org 3.10.0-957.10.1.el7.x86_64 x86_64)
//
// ------ Last repository commit info -----
// [ Date ]
// [ Author ]
// [ Source ]
// [ Revision ]

#include "JEventSource_toyDet.h"

//---------------------------------
// JEventSource_toyDet    (Constructor)
//---------------------------------
JEventSource_toyDet::JEventSource_toyDet(std::string source_name, JApplication *app):JEventSource(source_name, app)
{
  // Don't open the file/stream here. Do it in the Open() method below.
}

//---------------------------------
// ~JEventSource_toyDet    (Destructor)
//---------------------------------
JEventSource_toyDet::~JEventSource_toyDet()
{
  // Delete JFactoryGenerator if we created one
  if( mFactoryGenerator != nullptr ) delete mFactoryGenerator;

  // Close the file/stream here.
  cout << "Closing " << mName << " now in JEventSource_toyDet::~JEventSource_toyDet" << endl;
  ifs.close();
}

//------------------
// Open
//------------------
void JEventSource_toyDet::Open(void)
{
  // Open the file/stream here. The name of the source will be in mName
  ifs.open(mName);
  if (!ifs) {
    cout << "!!! Unable to open " << mName << " in JEventSource_toyDet::Open !!!" << endl;
    exit(1);  // terminate with error
  }
}

//------------------
// GetEvent
//------------------
// std::shared_ptr<const JEvent> JEventSource_toyDet::GetEvent(void)
void JEventSource_toyDet::GetEvent(std::shared_ptr<JEvent> event)
{

  // read an event from the input stream
  if (ifs.is_open()) {

    // initialize counters
    int lineCntr = 0; int chanCntr = 0; int eventCntr = 0;

    // read lines of input file
    while (getline(ifs, line)) {
  
      // locate event delimiter
      size_t eventDelim = line.find('@');
      if (eventDelim != string::npos) {
	// iterate and reset counters, store and clear data objects
	eventCntr++; chanCntr = 0;
	if (eventCntr > 1) eventData.push_back(chanData);
	tdcData.clear(); adcData.clear(); chanData.clear();
      }

      // locate channel delimiter
      size_t chanDelim = line.find('#');
      if (chanDelim != string::npos) {
     	// iterate and reset counters, clear data objects
        chanCntr++; lineCntr = 0;
	tdcData.clear(); adcData.clear();
      }

      lineCntr++;
      // aquire tdc data
      if (lineCntr > 0 && lineCntr % 2 == 0 && 
	  eventDelim == string::npos && chanDelim == string::npos) {
	istringstream istr(line);
	double tdcSample;
	while (!istr.eof()) {
	  istr >> tdcSample; if (!istr) break;
	  tdcData.push_back(tdcSample);
	}
        // cout << "tdc data" << endl;
        // for (auto sample : tdcData) cout << sample << endl;
      }
      // acquire adc data
      if (lineCntr > 0 && lineCntr % 2 != 0 && 
	  eventDelim == string::npos && chanDelim == string::npos) {
	istringstream istr(line);
	double adcSample;
	while (!istr.eof()) {
	  istr >> adcSample; if (!istr) break;
	  adcData.push_back(adcSample);
	}
        // cout << "adc data" << endl;	   
        // for (auto sample : adcData) cout << sample << endl;
      }

      size_t tdcDataSize = tdcData.size();
      size_t adcDataSize = adcData.size();      
      
      if (tdcDataSize == adcDataSize && tdcDataSize > 0) 
	chanData.push_back(new rawSamples(eventCntr, chanCntr, tdcData, adcData));

      line.clear();   
    }

    // when end of input file has been reached
    if (ifs.eof()) {
      // append data objects from last event
      eventData.push_back(chanData);
      cout << "Reached end of file/stream " << mName << endl;
      ifs.close();
    }
  }

  // Throw exception if we have exhausted the source of events
  static size_t ievent = 0; 
  if (ievent < eventData.size()) {
    ievent++;
    event->Insert(eventData[ievent-1]);
  }
  else throw JEventSource::RETURN_STATUS::kNO_MORE_EVENTS;

}

//------------------
// GetObjects
//------------------
bool JEventSource_toyDet::GetObjects(const std::shared_ptr<const JEvent>& aEvent, JFactory* aFactory)
{
  // This should create objects that are of the type aFactory holds using
  // data from aEvent. The objects should be transferred to aFactory which
  // serves simply as a container. Two possible paradigms are shown below
  // as OPTION 1 and OPTION 2

  // lock_guard<mutex> lck(vsamples_mutex);

  // auto vsamples = vsamples_deque.pop();
  // (*aEvent).Insert( vrawsamples );

  return true;

}

// The following method is optional in case you need to implement a more 
// complicated scheme. If not defined, a default JTask will be created for
// each event that simply runs all event processors on it. (This is
// probably what you want so feel free to delete this section.)
//
// An example of when you would implement this method is if you wanted
// to split the parsing of the input data  through multiple stages.
// For example, the first stage might simply split the larger buffer
// which contains many events into smaller buffers containing single
// events (e.g. disentangling). The JEventProcessor tasks should only
// be attached to single events rather than blocks so it makes sense
// to make the parsing of the blocks a different flavor of JTask.
// See the documentation for more details.
//------------------
// GetProcessEventTask
//------------------
// std::shared_ptr<JTaskBase> JEventSource_toyDet::GetProcessEventTask(std::shared_ptr<const JEvent>&& aEvent)
//{
// 
//}
