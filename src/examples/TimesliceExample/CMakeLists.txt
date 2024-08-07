

if (USE_PODIO)

    add_jana_plugin(TimesliceExample
        SOURCES TimesliceExample.cc CollectionTabulators.cc)

    target_link_libraries(TimesliceExample PUBLIC PodioExampleDatamodel PodioExampleDatamodelDict podio::podioRootIO)

    add_test(NAME jana-example-timeslices-simple-tests
        COMMAND jana -Pplugins=TimesliceExample -Pjana:nevents=10 events.root)

    add_test(NAME jana-example-timeslices-complex-tests
        COMMAND jana -Pplugins=TimesliceExample -Pjana:nevents=10 timeslices.root)

else()
    message(STATUS "Skipping examples/TimesliceExample because USE_PODIO=Off")
endif()


