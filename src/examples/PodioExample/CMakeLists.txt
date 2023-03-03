
set(PodioExample_SOURCES
        PodioExample.cc
        PodioExampleProcessor.cc
        PodioExampleSource.cc
        ExampleClusterFactory.cc
        ExampleMultifactory.cc
        )

if (USE_PODIO)

    foreach( _conf ${CMAKE_CONFIGURATION_TYPES} )
        string(TOUPPER ${_conf} _conf )
        set( CMAKE_RUNTIME_OUTPUT_DIRECTORY_${_conf} ${CMAKE_CURRENT_BINARY_DIR} )
        set( CMAKE_LIBRARY_OUTPUT_DIRECTORY_${_conf} ${CMAKE_CURRENT_BINARY_DIR} )
        set( CMAKE_ARCHIVE_OUTPUT_DIRECTORY_${_conf} ${CMAKE_CURRENT_BINARY_DIR} )
    endforeach()
    PODIO_GENERATE_DATAMODEL(datamodel layout.yaml DATAMODEL_HEADERS DATAMODEL_SOURCES IO_BACKEND_HANDLERS ROOT)
    PODIO_ADD_DATAMODEL_CORE_LIB(DataModelLib "${DATAMODEL_HEADERS}" "${DATAMODEL_SOURCES}")
    PODIO_ADD_ROOT_IO_DICT(DataModelDict DataModelLib "${DATAMODEL_HEADERS}" src/selection.xml)

    find_package(podio REQUIRED)
    add_executable(PodioExample ${PodioExample_SOURCES})
    target_include_directories(PodioExample PUBLIC .)
    target_link_libraries(PodioExample jana2 podio::podio DataModelLib DataModelDict podio::podioRootIO)
    set_target_properties(PodioExample PROPERTIES INSTALL_RPATH_USE_LINK_PATH TRUE)

    install(TARGETS PodioExample DESTINATION bin)
else()
    message(STATUS "Skipping examples/PodioExample because USE_PODIO=Off")

endif()



