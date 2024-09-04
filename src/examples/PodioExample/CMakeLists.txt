
set(PodioExample_SOURCES
        PodioExample.cc
        PodioExampleProcessor.cc
        ExampleClusterFactory.cc
        ExampleMultifactory.cc
        )

if (USE_PODIO)

    add_executable(PodioExample ${PodioExample_SOURCES})
    target_link_libraries(PodioExample 
        PUBLIC jana2 PodioDatamodel PodioDatamodelDict podio::podioRootIO)

    set_target_properties(PodioExample PROPERTIES INSTALL_RPATH_USE_LINK_PATH TRUE)

    install(TARGETS PodioExample DESTINATION bin)

    add_test(NAME jana-example-podio-tests COMMAND PodioExample)

else()
    message(STATUS "Skipping examples/PodioExample because USE_PODIO=Off")

endif()



