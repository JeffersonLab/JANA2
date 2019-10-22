//
//    File: RootProcessor.cc
// Created: Mon Aug 26 16:29:25 EDT 2019
// Creator: pooser (on Linux rudy.jlab.org 3.10.0-1062.el7.x86_64 x86_64)
//

#include "RootProcessor.h"
#include "ADCSample.h"

//---------------------------------
// RootProcessor    (Constructor)
//---------------------------------
RootProcessor::RootProcessor() = default;

//---------------------------------
// ~RootProcessor    (Destructor)
//---------------------------------
RootProcessor::~RootProcessor() {
    // close output root file
    outFile->Close();
}

//------------------
// Init
//------------------
void RootProcessor::Init() {

    // This is called once at program startup.
    // define root file
    outFileName = new TString("outFile.root");
    outFile     = new TFile(*outFileName, "RECREATE");
    outFile->cd();
    std::cout << "RootProcessor::Init -> Output ROOT file " << *outFileName << " created" << std::endl;
    // define trees and branches
    eventTree  = new TTree("ET", "Streaming Detector Event Data Tree");
    sampleTree = new TTree("ST", "Streaming Detector Sample Data Tree");
    nentries = 0;
    eventTree->Branch("event", &event);

    adc_samples_chans = new TBranch* [numChans];
    tdc_samples_chans = new TBranch* [numChans];

    for (uint ichan = 0; ichan < numChans; ichan++) {
        adc_samples_chans[ichan] = sampleTree->Branch(Form("adcSamplesChan_%d", ichan + 1), &adcSample);
        tdc_samples_chans[ichan] = sampleTree->Branch(Form("tdcSamplesChan_%d", ichan + 1), &tdcSample);
    }

    // update the root file
    outFile->Write();
    outFile->Flush();
    outFile->cd();
}

//------------------
// Process
//------------------
void RootProcessor::Process(const std::shared_ptr<const JEvent>& aEvent) {

    // get adc sample object for each event
    auto eventData = aEvent->Get<ADCSample>();
    // impose mutex lock
    lock_guard<mutex> lck(fillMutex);
    // for each adc sample in the current event
    for (auto sample : eventData) {
        // Insert this sample into the correct location in the ROOT tree.
        // This is effectively doing a transpose of the incoming DAS file
        // Correctness requires that our samples be ordered by increasing sample_id
        chan      = sample->channel_id + 1;
        event     = static_cast <uint> (aEvent->GetEventNumber());
        adcSample = sample->adc_value;
        tdcSample = (sample->sample_id + 1) + numSamples*(event - 1);
        adc_samples_chans[chan-1]->Fill();
        tdc_samples_chans[chan-1]->Fill();
    }

    // fill event tree and set nentries on sample tree
    eventTree->Fill();
    nentries += eventData.size()/numChans;
    sampleTree->SetEntries(nentries);
    // update the root file
    //outFile->Write();
    //outFile->Flush();
    //outFile->cd();
}

//------------------
// Finish
//------------------
void RootProcessor::Finish() {

    outFile->Write();
    // outFile->Write();

}
