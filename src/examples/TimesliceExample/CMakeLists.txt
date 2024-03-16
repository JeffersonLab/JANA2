

if (USE_PODIO)
    set (TimesliceExample_SOURCES
            TimesliceExample.cc
        )

    add_library(TimesliceExample SHARED ${TimesliceExample_SOURCES})
    target_link_libraries(TimesliceExample jana2 podio::podio PodioExampleDatamodel PodioExampleDatamodelDict podio::podioRootIO)
    set_target_properties(TimesliceExample PROPERTIES PREFIX "" SUFFIX ".so" OUTPUT_NAME "TimesliceExample")
    install(TARGETS TimesliceExample DESTINATION programs)

else()
    message(STATUS "Skipping examples/TimesliceExample because USE_PODIO=Off")
endif()

