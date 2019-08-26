//
//    File: RootProcessor.h
// Created: Mon Aug 26 16:29:25 EDT 2019
// Creator: pooser (on Linux rudy.jlab.org 3.10.0-1062.el7.x86_64 x86_64)
//

#ifndef _RootProcessor_toyDet_h_
#define _RootProcessor_toyDet_h_

#include <mutex>

#include <JANA/JEventProcessor.h>
#include <JANA/JEvent.h>

#include <TFile.h>
#include <TTree.h>

using namespace std;

//////////////////////////////////////////////////////////////////////////////////////////////////
/// Brief class description.
///
/// Detailed class description.
//////////////////////////////////////////////////////////////////////////////////////////////////
class RootProcessor : public JEventProcessor {

	public:

    RootProcessor();

    virtual ~RootProcessor();

    virtual void Init(void);

    virtual void Process(const std::shared_ptr<const JEvent>& aEvent);

    virtual void Finish(void);

	protected:
	
	
	private:

    // mutex objects
    mutex fillMutex;

    // root objects
    TFile *outFile;
    TTree *eventTree, *sampleTree;

    // user defined data types
    // TODO: put these somewhere that make sense ('global' parameter?)
    static const uint numChans   = 80;
    static const uint numSamples = 1024;

    uint chan, event, nentries;
    uint tdcSample, adcSample;

};

#endif // _RootProcessor_h_

