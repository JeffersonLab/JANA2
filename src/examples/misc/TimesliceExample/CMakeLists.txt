

if (USE_PODIO)

    add_jana_plugin(TimesliceExample)

    target_link_libraries(TimesliceExample PUBLIC PodioDatamodel PodioDatamodelDict podio::podioRootIO)

    add_test(NAME jana-example-timeslices-simple-tests
        COMMAND $<TARGET_FILE:jana> -Pplugins=TimesliceExample -Puse_timeslices=0 -Pjana:nevents=10 events.root)

    set_tests_properties(jana-example-timeslices-simple-tests PROPERTIES
        ENVIRONMENT "JANA_PLUGIN_PATH=${CMAKE_BINARY_DIR}/lib/JANA/plugins;LD_LIBRARY_PATH=$<TARGET_FILE_DIR:jana2_shared_lib>:$ENV{LD_LIBRARY_PATH}"
    )

    add_test(NAME jana-example-timeslices-complex-tests
        COMMAND $<TARGET_FILE:jana> -Pplugins=TimesliceExample -Puse_timeslices=1 -Pjana:nevents=10 timeslices.root)

    set_tests_properties(jana-example-timeslices-complex-tests PROPERTIES
        ENVIRONMENT "JANA_PLUGIN_PATH=${CMAKE_BINARY_DIR}/lib/JANA/plugins;LD_LIBRARY_PATH=$<TARGET_FILE_DIR:jana2_shared_lib>:$ENV{LD_LIBRARY_PATH}"
    )

else()
    message(STATUS "Skipping examples/TimesliceExample because USE_PODIO=Off")
endif()


