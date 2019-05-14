//
// Created by Nathan W Brei on 2019-03-10.
//

#define CATCH_CONFIG_MAIN
#include "catch.hpp"

#include <JANA/JServiceLocator.h>
#include "JServiceLocatorTests.h"


TEST_CASE("JServiceLocator chicken-and-egg tests") {

    JServiceLocator sl;

    // PHASE 1: jana.cc sets up params and logger services, which literally everything else needs

    auto parameterSvc = new ParameterSvc();
    parameterSvc->set("JANA:LogLevel", "DEBUG");   // Eventually set from program args or config file
    sl.provide(parameterSvc);     // Any errors parsing parameters are sent to the UI, not the logger

    auto loggerSvc_throwaway = new LoggerSvc();
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

    sl.provide(new MagneticFieldSvc());
    REQUIRE(parameterSvc->get("EIC:MagneticFieldDatabaseURL") == "");  // MagneticFieldSvc doesn't have its params yet


    // PHASE 5: Once all plugins are loaded and all Services are provided, we wire them all up _before_ any
    // processing starts. This involves JServiceLocator calling acquire_services() on everything
    // MagneticFieldService retrieves its database URL or sets a default which the user can see.
    sl.wire_everything();
    REQUIRE(parameterSvc->get("EIC:MagneticFieldDatabaseURL") == "mysql://127.0.0.1");

    // PHASE 6: Everything is ready, we can do whatever we want.
    // If the user specified --listconfigs, then we print all configs. This will include the
    // EIC:MagneticFieldServiceURL parameter

    auto magneticFieldSvc = sl.get<MagneticFieldSvc>();
    REQUIRE(magneticFieldSvc->connect() == true);

}




