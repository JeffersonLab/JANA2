
// Copyright 2020, Jefferson Science Associates, LLC.
// Subject to the terms in the LICENSE file found in the top-level directory.


#ifndef _RootProcessor_h_
#define _RootProcessor_h_

#include <mutex>

#include <JANA/JEventProcessor.h>

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
    ~RootProcessor() override;

    void Init() override;
    void Process(const std::shared_ptr<const JEvent>& aEvent) override;
    void Finish() override;

	protected:

	private:

    // mutex objects
    mutex fillMutex;

    // root objects
    TString *outFileName{};
    TFile *outFile{};
    TTree *eventTree{}, *sampleTree{};

    // user defined data types
    // TODO: put these somewhere that make sense ('global' parameter?)
    static const uint numChans   = 80;
    static const uint numSamples = 1024;

    uint chan{}, event{}, nentries{};
    uint tdcSample{}, adcSample{};

    TBranch** adc_samples_chans;
    TBranch** tdc_samples_chans;

};

#endif // _RootProcessor_h_

