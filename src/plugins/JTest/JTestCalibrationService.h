
// Copyright 2020, Jefferson Science Associates, LLC.
// Subject to the terms in the LICENSE file found in the top-level directory.

#ifndef JANA2_JTESTCALIBRATIONSERVICE_H
#define JANA2_JTESTCALIBRATIONSERVICE_H

#include <JANA/JService.h>

struct JTestCalibrationService: JService {

    Parameter<double> m_calibration_value{this, "calibration_value", 7.0, "Dummy calibration value"};

    double getCalibration() {
        return *m_calibration_value;
    }

};

#endif //JANA2_JTESTCALIBRATIONSERVICE_H
