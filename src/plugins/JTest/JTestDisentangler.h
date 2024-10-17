
// Copyright 2020, Jefferson Science Associates, LLC.
// Subject to the terms in the LICENSE file found in the top-level directory.

#ifndef JANA2_JTESTDISENTANGLER_H
#define JANA2_JTESTDISENTANGLER_H

#include <JANA/JFactoryT.h>
#include <JANA/JEvent.h>
#include <JANA/Utils/JBenchUtils.h>

#include "JTestDataObjects.h"
#include "JTestCalibrationService.h"


class JTestDisentangler : public JFactoryT<JTestEventData> {

    Parameter<size_t> m_cputime_ms {this, "cputime_ms", 20, "Time spent during disentangling" };
    Parameter<size_t> m_write_bytes {this, "bytes", 500000, "Bytes written during disentangling"};
    Parameter<double> m_cputime_spread {this, "cputime_spread", 0.25, "Spread of time spent during disentangling"};
    Parameter<double> m_write_spread {this, "bytes_spread", 0.25, "Spread of bytes written during disentangling"};

    Service<JTestCalibrationService> m_calibration_service {this};

    JBenchUtils m_bench_utils;

public:

    JTestDisentangler() {
        SetPrefix("disentangler");
        SetTypeName(NAME_OF_THIS);
    }

    void Process(const std::shared_ptr<const JEvent> &aEvent) override {

        m_bench_utils.set_seed(aEvent->GetEventNumber(), NAME_OF_THIS);

        // Read (large) entangled event data
        auto eed = aEvent->GetSingle<JTestEntangledEventData>();
        m_bench_utils.read_memory(*eed->buffer);

        // Read calibration data
        auto calib = m_calibration_service->getCalibration();

        // Do a little bit of computation
        m_bench_utils.consume_cpu_ms(*m_cputime_ms + calib, *m_cputime_spread);

        // Write (large) event data
        auto ed = new JTestEventData;
        m_bench_utils.write_memory(ed->buffer, *m_write_bytes, *m_write_spread);
        Insert(ed);
    }
};

#endif //JANA2_JTESTDISENTANGLER_H
