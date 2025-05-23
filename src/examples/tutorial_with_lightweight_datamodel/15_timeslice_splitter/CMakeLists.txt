

add_jana_plugin(podio_timeslice_splitter)

target_link_libraries(podio_timeslice_splitter PUBLIC jana2_tut_podio_datamodel)

add_test(NAME jana-example-timeslices-simple-tests
    COMMAND ${CMAKE_INSTALL_PREFIX}/bin/jana -Pplugins=podio_timeslice_splitter -Pjana:nevents=10 events.root)

add_test(NAME jana-example-timeslices-complex-tests
    COMMAND ${CMAKE_INSTALL_PREFIX}/bin/jana -Pplugins=podio_timeslice_splitter -Pjana:nevents=10 timeslices.root)


