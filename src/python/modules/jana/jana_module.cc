//
// Copyright 2020, Jefferson Science Associates, LLC.
// Subject to the terms in the LICENSE file found in the top-level directory.

#include <janapy.h>

//================================================================================
// Module definition
// The arguments of this structure tell Python what to call the extension,
// what it's methods are and where to look for it's method definitions.
// The routines themselves are all defined in src/python/common/janapy.h
// and src/python/common/janapy.cc.
PYBIND11_MODULE(jana, m) {

    m.doc() = "JANA2 Python Interface";

    // (see src/python/common/janapy.h)
    JANA_MODULE_DEF

    pyjapp = new JApplication();
    PY_MODULE_INSTANTIATED_JAPPLICATION = true;
}
