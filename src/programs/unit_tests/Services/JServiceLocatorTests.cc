
// Copyright 2020, Jefferson Science Associates, LLC.
// Subject to the terms in the LICENSE file found in the top-level directory.

#define CATCH_CONFIG_MAIN
#include "catch.hpp"

#include "JServiceLocatorTests.h"
#include <JANA/JService.h>
#include <JANA/Services/JServiceLocator.h>


TEST_CASE("JServiceLocatorMissingServiceTest") {
    JServiceLocator sut;
    sut.InitAllServices();
    REQUIRE_THROWS(sut.get<ParameterSvc>());
    /*
    try {
        sut.get<ParameterSvc>();
        REQUIRE(0 == 1); // Never gets here because it throws
    }
    catch (JException ex) {
        std::cout << ex << std::endl;
    }
    */
}

TEST_CASE("JServiceLocator chicken-and-egg tests") {

    JServiceLocator sl;

    // PHASE 1: jana.cc sets up params and logger services, which literally everything else needs

    auto parameterSvc = std::make_shared<ParameterSvc>();
    parameterSvc->set("JANA:LogLevel", "DEBUG");   // Eventually set from program args or config file
    sl.provide(parameterSvc);     // Any errors parsing parameters are sent to the UI, not the logger

    auto loggerSvc_throwaway = std::make_shared<LoggerSvc>();
    sl.provide(loggerSvc_throwaway);
    // LoggerSvc has not yet obtained its loglevel from parameterSvc
    REQUIRE(loggerSvc_throwaway->level == "INFO");

    // We need a logger ASAP so we can log failed plugin loading, etc
    // sl.get() will call logger.finalize(), which will call parameterSvc.acquire_services().
    // logger can get<>() the parameterSvc for the first time inside acquire_services()
    auto loggingSvc = sl.get<LoggerSvc>();
    REQUIRE(loggingSvc->level == "DEBUG");

    // PHASE 2: jana.cc registers other 'internal' services such as JApplication, JThreadManager, etc
    // We call JApplication::acquire_services() before plugin loading, where we pull in all of the other services
    // we need and make JApplication usable by others


    // PHASE 3: We call JApplication::loadPlugins(), which dlopens a bunch of plugins, each of which
    // has an InitPlugin() function which does something like:

    auto magFieldSvc = std::make_shared<MagneticFieldSvc>();
    sl.provide(magFieldSvc);
    REQUIRE(parameterSvc->get("EIC:MagneticFieldDatabaseURL") == "");  // MagneticFieldSvc doesn't have its params yet


    // PHASE 5: Once all plugins are loaded and all Services are provided, we initialize them _before_ any
    // processing starts. This involves JServiceLocator calling acquire_services() on everything
    // MagneticFieldService retrieves its database URL or sets a default which the user can see.
    sl.InitAllServices();
    REQUIRE(parameterSvc->get("EIC:MagneticFieldDatabaseURL") == "mysql://127.0.0.1");

    // PHASE 6: Everything is ready, we can do whatever we want.
    // If the user specified --listconfigs, then we print all configs. This will include the
    // EIC:MagneticFieldServiceURL parameter

    auto magneticFieldSvc = sl.get<MagneticFieldSvc>();
    REQUIRE(magneticFieldSvc->connect() == true);

}

struct OmniService : public JService {

    Service<JParameterManager> parman {this};
    Parameter<int> bucket_count {this, "bucket_count", 5, "Some integer representing a bucket count"};
    std::atomic_int init_call_count {0};

    void Init() override {
        LOG_INFO(GetLogger()) << "Calling OmniService::Init" << LOG_END;
        REQUIRE(parman->GetParameterValue<int>("bucket_count") == 22);
        REQUIRE(bucket_count() == 22);
        REQUIRE(init_call_count == 0);
        init_call_count++;
    }
};

TEST_CASE("JService Omni interface") {
    JApplication app;
    app.SetParameterValue("bucket_count", 22);
    app.ProvideService(std::make_shared<OmniService>());
    app.Initialize();
    auto sut = app.GetService<OmniService>();
    REQUIRE(sut->bucket_count() == 22);

    // Fetch again to make sure Init() is only called once
    sut = app.GetService<OmniService>();
    LOG << "Retrieved service " << sut << LOG_END; // Just in case the optimizer tries to get rid of this
}

