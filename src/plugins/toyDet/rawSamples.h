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

struct rawSamples : public JObject {

    rawSamples();
    virtual ~rawSamples();

    rawSamples(int eventNum, int chanNum, vector<double>& tdcData, vector<double>& adcData)
            : eventNum(eventNum), chanNum(chanNum), tdcData(tdcData), adcData(adcData) {}

    void Summarize(JObjectSummary& summary) const override {
        summary.add(eventNum, NAME_OF(eventNum), "%d");
        summary.add(chanNum, NAME_OF(chanNum), "%d");
        summary.add(adcData[0], "adcData[0]", "%1.9lf");
        summary.add(adcData[1], "adcData[1]", "%1.9lf");
        summary.add(adcData[2], "adcData[2]", "%1.9lf");
        summary.add(tdcData[0], "tdcData[0]", "%1.9lf");
        summary.add(tdcData[1], "tdcData[1]", "%1.9lf");
        summary.add(tdcData[2], "tdcData[2]", "%1.9lf");
    }


    int eventNum, chanNum;
    vector<double> tdcData, adcData;

};

#endif // _rawSamples_h_

