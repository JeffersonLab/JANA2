

if (USE_PODIO)

    add_jana_plugin(TimesliceExample
        SOURCES TimesliceExample.cc CollectionTabulators.cc)

    target_link_libraries(TimesliceExample PUBLIC PodioExampleDatamodel PodioExampleDatamodelDict podio::podioRootIO)

else()
    message(STATUS "Skipping examples/TimesliceExample because USE_PODIO=Off")
endif()

