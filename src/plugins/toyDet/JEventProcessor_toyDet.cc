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
#include "FECSample.h"

//---------------------------------
// JEventProcessor_toyDet    (Constructor)
//---------------------------------
JEventProcessor_toyDet::JEventProcessor_toyDet() {

}

//---------------------------------
// ~JEventProcessor_toyDet    (Destructor)
//---------------------------------
JEventProcessor_toyDet::~JEventProcessor_toyDet() {
    // close output root file
    outFile->Close();
}

//------------------
// Init
//------------------
void JEventProcessor_toyDet::Init(void) {
    // This is called once at program startup.

    std::cout << "Initializing ROOT file" << std::endl;
    // define root file
    outFile = new TFile("outFile.root", "RECREATE");
    // define trees and branches
    eventTree = new TTree("ET", "Toy Detector Event Data Tree");
    sampleTree = new TTree("ST", "Toy Detector Sample Data Tree");
    nentries = 0;
    eventTree->Branch("event", &event);
    for (uint ichan = 0; ichan < numChans; ichan++) {
        sampleTree->Branch(Form("chan_%d_adcSamples", ichan + 1), &adcSample);
    }
}

//------------------
// Process
//------------------
void JEventProcessor_toyDet::Process(const std::shared_ptr<const JEvent>& aEvent) {

    // get raw samples object for each event
    auto eventData = aEvent->Get<FECSample>();
    // impose mutex lock
    lock_guard<mutex> lck(fillMutex);

    // For each ADC sample in the current event
    for (auto sample : eventData) {

        // Insert this sample into the correct location in the ROOT tree.
        // This is effectively doing a transpose of the incoming DAS file
        // Correctness requires that our samples be ordered by increasing sample_id

        event = aEvent->GetEventNumber();
        chan = sample->channel_id + 1;
        adcSample = sample->adc_value;

        sampleTree->FindBranch(Form("chan_%d_adcSamples", chan))->Fill();
    }

    // fill event tree and set nentries on sample tree
    eventTree->Fill();
    nentries += eventData.size();
    sampleTree->SetEntries(nentries);

}

//------------------
// Finish
//------------------
void JEventProcessor_toyDet::Finish(void) {
    outFile->Write();
}
