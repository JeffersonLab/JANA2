
set(PodioExample_SOURCES
        PodioExample.cc
        PodioExampleProcessor.cc
        PodioExampleSource.cc
        ExampleClusterFactory.cc
        ExampleMultifactory.cc
        )

if (USE_PODIO)

    PODIO_GENERATE_DATAMODEL(PodioExampleDatamodel layout.yaml headers sources 
        IO_BACKEND_HANDLERS ROOT
        OUTPUT_FOLDER ${CMAKE_CURRENT_BINARY_DIR}
    )

    PODIO_ADD_DATAMODEL_CORE_LIB(PodioExampleDatamodel "${headers}" "${sources}"
        OUTPUT_FOLDER ${CMAKE_CURRENT_BINARY_DIR})

    PODIO_ADD_ROOT_IO_DICT(PodioExampleDatamodelDict PodioExampleDatamodel "${headers}" 
        ${CMAKE_CURRENT_BINARY_DIR}/src/selection.xml)

    find_package(podio REQUIRED)
    add_executable(PodioExample ${PodioExample_SOURCES})
    target_link_libraries(PodioExample 
        jana2 PodioExampleDatamodel PodioExampleDatamodelDict podio::podioRootIO)

    set_target_properties(PodioExample PROPERTIES INSTALL_RPATH_USE_LINK_PATH TRUE)

    install(TARGETS PodioExample DESTINATION bin)

    install(TARGETS PodioExampleDatamodel
        LIBRARY DESTINATION lib/JANA/plugins
        PUBLIC_HEADER DESTINATION include/JANA/plugins/PodioExampleDatamodel
    )
    
    install(TARGETS PodioExampleDatamodelDict DESTINATION lib/JANA/plugins)

    add_test(NAME jana-example-podio-tests
        COMMAND PodioExample)

    set_tests_properties(jana-example-podio-tests PROPERTIES DISABLED TRUE)

else()
    message(STATUS "Skipping examples/PodioExample because USE_PODIO=Off")

endif()



