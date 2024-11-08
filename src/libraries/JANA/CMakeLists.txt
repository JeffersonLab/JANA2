

# Everything else happens below
set(JANA2_SOURCES

    JApplication.cc
    JEvent.cc
    JEventSource.cc
    JFactory.cc
    JFactorySet.cc
    JMultifactory.cc
    JService.cc
    JVersion.cc
    JEvent.cc

    Engine/JExecutionEngine.cc

    Topology/JArrow.cc
    Topology/JEventSourceArrow.cc
    Topology/JEventMapArrow.cc
    Topology/JEventTapArrow.cc
    Topology/JTopologyBuilder.cc

    Services/JComponentManager.cc
    Services/JParameterManager.cc
    Services/JPluginLoader.cc
    Services/JWiringService.cc

    Components/JComponentSummary.cc

    Utils/JCpuInfo.cc
    Utils/JProcessorMapping.cc
    Utils/JBenchUtils.cc
    Utils/JStringification.cc
    Utils/JAutoActivator.cc
    Utils/JTablePrinter.cc
    Utils/JCallGraphRecorder.cc
    Utils/JInspector.cc
    Utils/JApplicationInspector.cc

    Calibrations/JCalibration.cc
    Calibrations/JCalibrationFile.cc
    Calibrations/JLargeCalibration.cc

    CLI/JBenchmarker.cc
    CLI/JSignalHandler.cc
    CLI/JMain.cc

    Compatibility/JStreamLog.cc
    Compatibility/JStreamLogBuffer.cc
    Compatibility/JGeometryManager.cc
    Compatibility/JGeometryXML.cc
    Compatibility/md5.c
    )

if (${USE_PODIO})
    list(APPEND JANA2_SOURCES
            Podio/JFactoryPodioT.cc
        )
endif()

if (NOT ${USE_XERCES})
    message(STATUS "Skipping support for libJANA's JGeometryXML because USE_XERCES=Off")
endif()

add_library(jana2 OBJECT ${JANA2_SOURCES})

find_package(Threads REQUIRED)
set(THREADS_PREFER_PTHREAD_FLAG ON)
target_link_libraries(jana2 PUBLIC ${CMAKE_DL_LIBS} Threads::Threads)
target_link_libraries(jana2 PRIVATE VendoredTomlPlusPlus)

if (${USE_PODIO})
    target_link_libraries(jana2 PUBLIC podio::podio podio::podioRootIO ${ROOT_LIBRARIES})
elseif (${USE_ROOT})
    target_link_libraries(jana2 PUBLIC ${ROOT_LIBRARIES})
endif()


# static library, always there
add_library(jana2_static_lib STATIC $<TARGET_OBJECTS:jana2>)
set_target_properties(jana2_static_lib PROPERTIES PREFIX "lib" OUTPUT_NAME "JANA")

target_include_directories(jana2_static_lib PUBLIC $<INSTALL_INTERFACE:include>)
target_link_libraries(jana2_static_lib PUBLIC ${CMAKE_DL_LIBS} Threads::Threads)
target_link_libraries(jana2_static_lib PUBLIC VendoredTomlPlusPlus)

if (${USE_PODIO})
    target_link_libraries(jana2_static_lib PUBLIC podio::podio podio::podioRootIO ${ROOT_LIBRARIES})
elseif (${USE_ROOT})
    target_link_libraries(jana2_static_lib PUBLIC ${ROOT_LIBRARIES})
endif()
install(TARGETS jana2_static_lib EXPORT jana2_targets DESTINATION lib)

# optionally build shared lib
if (BUILD_SHARED_LIBS)
    message(STATUS "Building both shared and static libraries")
    add_library(jana2_shared_lib SHARED $<TARGET_OBJECTS:jana2>)
    set_target_properties(jana2_shared_lib PROPERTIES PREFIX "lib" OUTPUT_NAME "JANA")

    target_include_directories(jana2_shared_lib PUBLIC $<INSTALL_INTERFACE:include>)
    target_link_libraries(jana2_shared_lib PUBLIC ${CMAKE_DL_LIBS} Threads::Threads)
    target_link_libraries(jana2_shared_lib PUBLIC VendoredTomlPlusPlus)

    if (${USE_PODIO})
        target_link_libraries(jana2_shared_lib PUBLIC podio::podio podio::podioRootIO ${ROOT_LIBRARIES})
    elseif (${USE_ROOT})
        target_link_libraries(jana2_shared_lib PUBLIC ${ROOT_LIBRARIES})
    endif()

    install(TARGETS jana2_shared_lib EXPORT jana2_targets DESTINATION lib)
    set(INSTALL_RPATH_USE_LINK_PATH True)
else()
    message(STATUS "Building static library only")
endif()


# Install "public" header files

file(GLOB jana_headers "*.h*")
file(GLOB jana_engine_headers "Engine/*.h*")
file(GLOB jana_services_headers "Services/*.h*")
file(GLOB jana_streaming_headers "Streaming/*.h*")
file(GLOB jana_utils_headers "Utils/*.h*")
file(GLOB jana_calibs_headers "Calibrations/*.h*")
file(GLOB jana_cli_headers "CLI/*.h*")
file(GLOB jana_compat_headers "Compatibility/*.h*")
file(GLOB jana_podio_headers "Podio/*.h*")
file(GLOB jana_component_headers "Components/*.h*")
file(GLOB jana_topology_headers "Topology/*.h*")

install(FILES ${jana_headers} DESTINATION include/JANA)
install(FILES ${jana_engine_headers} DESTINATION include/JANA/Engine)
install(FILES ${jana_services_headers} DESTINATION include/JANA/Services)
install(FILES ${jana_streaming_headers} DESTINATION include/JANA/Streaming)
install(FILES ${jana_utils_headers} DESTINATION include/JANA/Utils)
install(FILES ${jana_calibs_headers} DESTINATION include/JANA/Calibrations)
install(FILES ${jana_cli_headers} DESTINATION include/JANA/CLI)
install(FILES ${jana_compat_headers} DESTINATION include/JANA/Compatibility)
install(FILES ${jana_component_headers} DESTINATION include/JANA/Components)
install(FILES ${jana_topology_headers} DESTINATION include/JANA/Topology)

if (${USE_PODIO})
    install(FILES ${jana_podio_headers} DESTINATION include/JANA/Podio)
endif()






