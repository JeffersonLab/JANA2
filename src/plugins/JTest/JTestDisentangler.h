
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

    size_t m_cputime_ms = 20;
    size_t m_write_bytes = 500000;
    double m_cputime_spread = 0.25;
    double m_write_spread = 0.25;
    JBenchUtils m_bench_utils = JBenchUtils();

    std::shared_ptr<JTestCalibrationService> m_calibration_service;

public:

    void Init() override {
        auto app = GetApplication();
        assert (app != nullptr);
        app->SetDefaultParameter("jtest:disentangler_ms", m_cputime_ms, "Time spent during disentangling");
        app->SetDefaultParameter("jtest:disentangler_spread", m_cputime_spread, "Spread of time spent during disentangling");
        app->SetDefaultParameter("jtest:disentangler_bytes", m_write_bytes, "Bytes written during disentangling");
        app->SetDefaultParameter("jtest:disentangler_bytes_spread", m_write_spread, "Spread of bytes written during disentangling");

        // Retrieve calibration service from JApp
        m_calibration_service = app->GetService<JTestCalibrationService>();
    }

    void Process(const std::shared_ptr<const JEvent> &aEvent) override {

        m_bench_utils.set_seed(aEvent->GetEventNumber(), NAME_OF_THIS);
        // Read (large) entangled event data
        auto eed = aEvent->GetSingle<JTestEntangledEventData>();
        m_bench_utils.read_memory(*eed->buffer);

        // Read calibration data
        m_calibration_service->getCalibration();

        // Do a little bit of computation
        m_bench_utils.consume_cpu_ms(m_cputime_ms, m_cputime_spread);

        // Write (large) event data
        auto ed = new JTestEventData;
        m_bench_utils.write_memory(ed->buffer, m_write_bytes, m_write_spread);
        Insert(ed);
    }
};

#endif //JANA2_JTESTDISENTANGLER_H
