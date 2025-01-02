
// Copyright 2007-2025, Jefferson Science Associates, LLC.
// Subject to the terms in the LICENSE file found in the top-level directory.
// Author: David Lawrence

#pragma once
#include <JANA/Services/JServiceLocator.h>
#include <JANA/Geometry/JGeometry.h>

#include <mutex>
#include <vector>

class JGeometryManager: public JService {

    std::mutex m_mutex;
    std::vector<JGeometry*> geometries;

public:
    JGeometry* GetJGeometry(unsigned int run_number);

};


