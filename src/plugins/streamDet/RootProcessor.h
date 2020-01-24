//
//    File: RootProcessor.h
// Created: Mon Aug 26 16:29:25 EDT 2019
// Creator: pooser (on Linux rudy.jlab.org 3.10.0-1062.el7.x86_64 x86_64)
//

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

};

#endif // _RootProcessor_h_

