

if (USE_PODIO)

    add_jana_plugin(TimesliceExample)

    target_link_libraries(TimesliceExample PUBLIC PodioDatamodel PodioDatamodelDict podio::podioRootIO)

    add_test(NAME jana-example-timeslices-simple-tests
        COMMAND $<TARGET_FILE:jana> -Pplugins=TimesliceExample -Puse_timeslices=0 -Pjana:nevents=10 events.root)

    add_test(NAME jana-example-timeslices-complex-tests
        COMMAND $<TARGET_FILE:jana> -Pplugins=TimesliceExample -Puse_timeslices=1 -Pjana:nevents=10 timeslices.root)

else()
    message(STATUS "Skipping examples/TimesliceExample because USE_PODIO=Off")
endif()


