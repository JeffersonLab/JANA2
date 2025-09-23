
// Copyright 2020, Jefferson Science Associates, LLC.
// Subject to the terms in the LICENSE file found in the top-level directory.

#ifndef JTestPlotter_h
#define JTestPlotter_h

#include <JANA/JEventProcessor.h>
#include <JANA/Utils/JBenchUtils.h>
#include "JTestDataObjects.h"

class JTestPlotter : public JEventProcessor {

    Parameter<size_t> m_cputime_ms {this, "cputime_ms", 0, "Time spent during plotting" };
    Parameter<size_t> m_write_bytes {this, "bytes", 1000, "Bytes written during plotting"};
    Parameter<double> m_cputime_spread {this, "cputime_spread", 0.25, "Spread of time spent during plotting"};
    Parameter<double> m_write_spread {this, "bytes_spread", 0.25, "Spread of bytes written during plotting"};

    Input<JTestTrackData> m_track_data {this};
    Input<JTestTrackAuxilliaryData> m_track_aux_data {this};

    JBenchUtils m_bench_utils;

public:

    JTestPlotter(bool order_output=false) {
        SetPrefix("jtest:plotter");
        SetTypeName(NAME_OF_THIS);
        SetCallbackStyle(CallbackStyle::ExpertMode);
        EnableOrdering(order_output);
    }

    void ProcessSequential(const JEvent& event) override {

        m_bench_utils.set_seed(event.GetEventNumber(), typeid(*this).name());

        // Read the track data
        m_bench_utils.read_memory(m_track_data->at(0)->buffer);

        // Consume CPU
        m_bench_utils.consume_cpu_ms(*m_cputime_ms + m_track_aux_data->at(0)->something, *m_cputime_spread);

        // Write the histogram data
        auto hd = new JTestHistogramData;
        m_bench_utils.write_memory(hd->buffer, *m_write_bytes, *m_write_spread);
        event.Insert(hd);
    }

};

#endif // JTestPlotter_h

