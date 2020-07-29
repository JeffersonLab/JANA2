
// Copyright 2020, Jefferson Science Associates, LLC.
// Subject to the terms in the LICENSE file found in the top-level directory.

#ifndef JANA2_JTESTCALIBRATIONSERVICE_H
#define JANA2_JTESTCALIBRATIONSERVICE_H

struct JTestCalibrationService: JService {
    double getCalibration() {
        return 7.0;
    }

};

#endif //JANA2_JTESTCALIBRATIONSERVICE_H
