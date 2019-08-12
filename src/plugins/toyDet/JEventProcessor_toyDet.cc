//
//    File: toyDet/JEventProcessor_toyDet.cc
// Created: Wed Apr 24 16:04:14 EDT 2019
// Creator: pooser (on Linux rudy.jlab.org 3.10.0-957.10.1.el7.x86_64 x86_64)
//
// ------ Last repository commit info -----
// [ Date ]
// [ Author ]
// [ Source ]
// [ Revision ]

#include "JEventProcessor_toyDet.h"
#include "rawSamples.h"

//---------------------------------
// JEventProcessor_toyDet    (Constructor)
//---------------------------------
JEventProcessor_toyDet::JEventProcessor_toyDet()
{

}

//---------------------------------
// ~JEventProcessor_toyDet    (Destructor)
//---------------------------------
JEventProcessor_toyDet::~JEventProcessor_toyDet()
{
  // close output root file
  outFile->Close();
}

//------------------
// Init
//------------------
void JEventProcessor_toyDet::Init(void)
{
  // This is called once at program startup.

  // define root file
  outFile = new TFile("outFile.root", "RECREATE");
  // define trees and branches
  eventTree  = new TTree("ET", "Toy Detector Event Data Tree");
  sampleTree = new TTree("ST", "Toy Detector Sample Data Tree");
  nentries = 0;
  eventTree->Branch("event", &event); 
  for (uint ichan = 0; ichan < numChans; ichan++) {
    sampleTree->Branch(Form("chan_%d_tdcSamples", ichan+1), &tdcSample);
    sampleTree->Branch(Form("chan_%d_adcSamples", ichan+1), &adcSample);   
  }
}

//------------------
// Process
//------------------
void JEventProcessor_toyDet::Process(const std::shared_ptr<const JEvent>& aEvent)
{
  // This is called for every event. Use of common resources like writing
  // to a file or filling a histogram should be mutex protected. Using
  // aEvent->Get<type>() to get reconstructed objects (and thereby activating the
  // reconstruction algorithm) should be done outside of any mutex lock
  // since multiple threads may call this method at the same time.
  // Here's an example:
  //
  // auto myTracks = aEvent->Get*<MyTrack>();
  //
  // lock_guard<mutex> lck( mymutex );
  // for( auto t : myTracks ){
  //  ... fill histograms or trees ...
  // }

  // get raw samples object for each event
  auto eventData = aEvent->Get<rawSamples>();
  // impose mutex lock
  lock_guard<mutex> lck(fillMutex);
  
  // for each event loop over the channel data
  for (auto chanData : eventData) {
   
    // acquire event and channel data
    chan  = chanData->chanNum;
    event = chanData->eventNum;
    tdcSamples.clear(); tdcSamples = chanData->tdcData;
    adcSamples.clear(); adcSamples = chanData->adcData;
    // get sample data and fill channel branches
    for (auto tdc : tdcSamples) {
      tdcSample = tdc;
      sampleTree->FindBranch(Form("chan_%d_tdcSamples", chan))->Fill();
    }
    for (auto adc : adcSamples) {
      adcSample = adc;
      sampleTree->FindBranch(Form("chan_%d_adcSamples", chan))->Fill();
    }

  }

  // fill event tree and set entires on sample tree
  eventTree->Fill();
  nentries += tdcSamples.size();
  sampleTree->SetEntries(nentries);

}

//------------------
// Finish
//------------------
void JEventProcessor_toyDet::Finish(void)
{
  // This is called when at the end of event processing
  outFile->Write();
}
