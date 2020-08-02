
// Copyright 2020, Jefferson Science Associates, LLC.
// Subject to the terms in the LICENSE file found in the top-level directory.

#ifndef JANA2_JTESTDISENTANGLER_H
#define JANA2_JTESTDISENTANGLER_H

#include <JANA/JFactoryT.h>
#include <JANA/JEvent.h>
#include <JANA/Utils/JPerfUtils.h>

#include "JTestDataObjects.h"
#include "JTestCalibrationService.h"


class JTestDisentangler : public JFactoryT<JTestEventData> {

    size_t m_cputime_ms = 20;
    size_t m_write_bytes = 500000;
    double m_cputime_spread = 0.25;
    double m_write_spread = 0.25;

    std::shared_ptr<JTestCalibrationService> m_calibration_service;

public:

    JTestDisentangler() : JFactoryT<JTestEventData>("JTestDisentangler") {};

    void Init() override {
        auto app = GetApplication();
        assert (app != nullptr);
        app->GetParameter("jtest:disentangler_bytes", m_write_bytes);
        app->GetParameter("jtest:disentangler_ms", m_cputime_ms);
        app->GetParameter("jtest:disentangler_bytes_spread", m_write_spread);
        app->GetParameter("jtest:disentangler_spread", m_cputime_spread);

        // Retrieve calibration service from JApp
        m_calibration_service = app->GetService<JTestCalibrationService>();
    }

    void Process(const std::shared_ptr<const JEvent> &aEvent) override {

        // Read (large) entangled event data
        auto eed = aEvent->GetSingle<JTestEntangledEventData>();
        read_memory(*eed->buffer);

        // Read calibration data
        m_calibration_service->getCalibration();

        // Do a little bit of computation
        consume_cpu_ms(m_cputime_ms, m_cputime_spread);

        // Write (large) event data
        auto ed = new JTestEventData;
        write_memory(ed->buffer, m_write_bytes, m_write_spread);
        Insert(ed);
    }
};

#endif //JANA2_JTESTDISENTANGLER_H
