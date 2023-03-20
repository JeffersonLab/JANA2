//
// Copyright 2020, Jefferson Science Associates, LLC.
// Subject to the terms in the LICENSE file found in the top-level directory.

#include <janapy.h>
#include <JANA/CLI/JMain.h>
#include <JANA/CLI/JVersion.h>

// Something to throw that makes a nicer error message
class PYTHON_MODULE_STARTUP_FAILED{public: PYTHON_MODULE_STARTUP_FAILED(){}};

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

    // This is a bit tricky:
    // Python modules are open by the python executable using dlopen,
    // but without the RTLD_GLOBAL flag. This means the symbols in
    // the module itself are not available for linking to additional
    // shared objects opened with dlopen. Specifically, when the python
    // script is run and it tries to attach a plugin, the plugin will
    // need access to JANA library routines and they cannot come from
    // the python module itself. Thus, we explicitly dlopen libJANA.so
    // here using the RTLD_GLOBAL flag.
    //
    // Note that this will not work if the shared library is not created
    // (by virtue of the BUILD_SHARED_LIBS cmake variable being set to
    // Off).

    std::string suffix = ".so";
#if __APPLE__
    suffix = ".dylib";
#endif

    auto jana_install_dir = JVersion::GetInstallDir();
    auto shared_lib = jana_install_dir + "/lib/libJANA" + suffix;
    void* handle = dlopen(shared_lib.c_str(), RTLD_LAZY | RTLD_GLOBAL | RTLD_NODELETE);
    if (!handle) {
        LOG_ERROR(default_cerr_logger) << dlerror() << LOG_END;
        LOG_ERROR(default_cerr_logger) << "This may be due to building JANA with BUILD_SHARED_LIBS=Off." << LOG_END;
        LOG_ERROR(default_cerr_logger) << "You can try running with the embedded python interpreter like this:" << LOG_END;
        LOG_ERROR(default_cerr_logger) << LOG_END;
        LOG_ERROR(default_cerr_logger) << "    jana -Pplugins=janapy -PJANA_PYTHON_FILE=myfile.py" << LOG_END;
        LOG_ERROR(default_cerr_logger) << LOG_END;
        LOG_ERROR(default_cerr_logger) << "Alternatively, build JANA with BUILD_SHARED_LIBS=On" << LOG_END;

        throw PYTHON_MODULE_STARTUP_FAILED();
    }else{
        // LOG <<"Opened " << shared_lib << LOG_END;
    }
    auto options = jana::ParseCommandLineOptions(0, nullptr, false);
    pyjapp = jana::CreateJApplication(options);

    PY_MODULE_INSTANTIATED_JAPPLICATION = true;
}
