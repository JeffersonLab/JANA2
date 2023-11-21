
# For now all the CMakeLists in 'CLI' does is generate and install JVersion.h
add_subdirectory(CLI)

# Everything else happens below
set(JANA2_SOURCES

    JApplication.cc
    JApplication.h
    JEvent.h
    JEventProcessor.h
    JEventSource.h
    JEventSourceGenerator.h
    JEventSourceGeneratorT.h
    JException.h
    JFactory.h
    JFactory.cc
    JFactoryGenerator.h
    JFactorySet.cc
    JFactorySet.h
    JFactoryT.h
    JObject.h
    JCsvWriter.h
    JLogger.h
    JMultifactory.cc
    JMultifactory.h

    Engine/JArrow.h
    Engine/JArrowMetrics.h
    Engine/JArrowPerfSummary.cc
    Engine/JArrowPerfSummary.h
    Engine/JArrowProcessingController.cc
    Engine/JArrowProcessingController.h
    Engine/JArrowTopology.cc
    Engine/JArrowTopology.h
    Engine/JDebugProcessingController.cc
    Engine/JDebugProcessingController.h
    Engine/JEventProcessorArrow.cc
    Engine/JEventProcessorArrow.h
    Engine/JEventSourceArrow.cc
    Engine/JEventSourceArrow.h
    Engine/JBlockSourceArrow.h
    Engine/JBlockDisentanglerArrow.h

    Engine/JMailbox.h
    Engine/JScheduler.cc
    Engine/JScheduler.h
    Engine/JSubeventArrow.h
    Engine/JWorker.h
    Engine/JWorker.cc
    Engine/JWorkerMetrics.h
    Engine/JTopologyBuilder.h

    Services/JComponentManager.cc
    Services/JComponentManager.h
    Services/JGlobalRootLock.h
    Services/JLoggingService.h
    Services/JParameterManager.cc
    Services/JParameterManager.h
    Services/JPluginLoader.cc
    Services/JPluginLoader.h
    Services/JProcessingController.h
    Services/JServiceLocator.h
    Services/JEventGroupTracker.h

    Status/JComponentSummary.h
    Status/JComponentSummary.cc
    Status/JPerfMetrics.cc
    Status/JPerfMetrics.h
    Status/JPerfSummary.h

    Streaming/JDiscreteJoin.h
    Streaming/JEventBuilder.h
    Streaming/JMessage.h
    Streaming/JStreamingEventSource.h
    Streaming/JTransport.h
    Streaming/JTrigger.h
    Streaming/JWindow.h

    Utils/JBacktrace.h
    Utils/JEventPool.h
    Utils/JCpuInfo.cc
    Utils/JCpuInfo.h
    Utils/JTypeInfo.h
    Utils/JResourcePool.h
    Utils/JResettable.h
    Utils/JProcessorMapping.h
    Utils/JProcessorMapping.cc
    Utils/JPerfUtils.cc
    Utils/JPerfUtils.h
    Utils/JStringification.cc
    Utils/JStringification.h
    Utils/JAutoActivator.cc
    Utils/JAutoActivator.h
    Utils/JAny.h
    Utils/JTablePrinter.cc
    Utils/JTablePrinter.h
    Utils/JCallGraphRecorder.h
    Utils/JCallGraphRecorder.cc
    Utils/JCallGraphEntryMaker.h
    Utils/JInspector.cc
    Utils/JInspector.h

    Calibrations/JCalibration.cc
    Calibrations/JCalibration.h
    Calibrations/JCalibrationCCDB.h
    Calibrations/JCalibrationFile.cc
    Calibrations/JCalibrationFile.h
    Calibrations/JCalibrationGenerator.h
    Calibrations/JCalibrationGeneratorCCDB.h
    Calibrations/JLargeCalibration.cc
    Calibrations/JLargeCalibration.h

    CLI/JBenchmarker.cc
    CLI/JBenchmarker.h
    CLI/JSignalHandler.cc
    CLI/JSignalHandler.h
    CLI/JMain.cc
    CLI/JMain.h

    Compatibility/JStreamLog.cc
    Compatibility/JStreamLog.h
    Compatibility/JStreamLogBuffer.cc
    Compatibility/JStreamLogBuffer.h
    Compatibility/JStatusBits.h
    Compatibility/jerror.h
    Compatibility/JGeometry.h
    Compatibility/JGeometryManager.cc
    Compatibility/JGeometryManager.h
    Compatibility/JGeometryMYSQL.cc
    Compatibility/JGeometryMYSQL.h
    Compatibility/JGeometryXML.cc
    Compatibility/JGeometryXML.h
    Compatibility/md5.c
    Compatibility/md5.h
    Compatibility/JLockService.h
    Compatibility/JGetObjectsFactory.h
    )

if (${USE_PODIO})
    list(APPEND JANA2_SOURCES
            Podio/JEventProcessorPodio.cc
            Podio/JFactoryPodioT.cc
        )
endif()

if (NOT ${USE_XERCES})
    message(STATUS "Skipping support for libJANA's JGeometryXML because USE_XERCES=Off")
endif()

add_library(jana2 OBJECT ${JANA2_SOURCES})

find_package(Threads REQUIRED)
set(THREADS_PREFER_PTHREAD_FLAG ON)
target_include_directories(jana2 PRIVATE ${CMAKE_CURRENT_BINARY_DIR}/..)
# So that we can find the generated JVersion.h before install
target_link_libraries(jana2 ${CMAKE_DL_LIBS} Threads::Threads)
if (${USE_PODIO})
    target_compile_definitions(jana2 PUBLIC JANA2_HAVE_PODIO=1 JANA2_HAVE_ROOT=1)
    target_link_libraries(jana2 podio::podio podio::podioRootIO ${ROOT_LIBRARIES})
elseif (${USE_ROOT})
    target_compile_definitions(jana2 PUBLIC JANA2_HAVE_ROOT=1)
    target_link_libraries(jana2 ${ROOT_LIBRARIES})
endif()


# static library, always there
add_library(jana2_static_lib STATIC $<TARGET_OBJECTS:jana2>)
set_target_properties(jana2_static_lib PROPERTIES PREFIX "lib" OUTPUT_NAME "JANA")

target_include_directories(jana2_static_lib PUBLIC $<INSTALL_INTERFACE:include>)
target_link_libraries(jana2_static_lib ${CMAKE_DL_LIBS} Threads::Threads)

if (${USE_PODIO})
    target_compile_definitions(jana2_static_lib PUBLIC JANA2_HAVE_PODIO=1 JANA2_HAVE_ROOT=1)
    target_link_libraries(jana2_static_lib podio::podio podio::podioRootIO ${ROOT_LIBRARIES})
elseif (${USE_ROOT})
    target_compile_definitions(jana2_static_lib PUBLIC JANA2_HAVE_ROOT=1)
    target_link_libraries(jana2_static_lib ${ROOT_LIBRARIES})
endif()
install(TARGETS jana2_static_lib EXPORT jana2_targets DESTINATION lib)


# optionally build shared lib
if (BUILD_SHARED_LIBS)
    message(STATUS "Building both shared and static libraries")
    add_library(jana2_shared_lib SHARED $<TARGET_OBJECTS:jana2>)
    set_target_properties(jana2_shared_lib PROPERTIES PREFIX "lib" OUTPUT_NAME "JANA")

    target_include_directories(jana2_shared_lib PUBLIC $<INSTALL_INTERFACE:include>)
    target_link_libraries(jana2_shared_lib ${CMAKE_DL_LIBS} Threads::Threads)

    if (${USE_PODIO})
        target_compile_definitions(jana2_shared_lib PUBLIC JANA2_HAVE_PODIO=1 JANA2_HAVE_ROOT=1)
        target_link_libraries(jana2_shared_lib podio::podio podio::podioRootIO ${ROOT_LIBRARIES})
    elseif (${USE_ROOT})
        target_compile_definitions(jana2_shared_lib PUBLIC JANA2_HAVE_ROOT=1)
        target_link_libraries(jana2_shared_lib ${ROOT_LIBRARIES})
    endif()

    install(TARGETS jana2_shared_lib EXPORT jana2_targets DESTINATION lib)
    set(INSTALL_RPATH_USE_LINK_PATH True)
else()
    message(STATUS "Building static library only")
endif()


# Install JANATargets.cmake
install(EXPORT jana2_targets FILE JANATargets.cmake NAMESPACE JANA:: DESTINATION lib/cmake/JANA)

# Generate JANAConfig.cmake, pointing it to JANATargets
include(CMakePackageConfigHelpers)
configure_package_config_file(
        "JANAConfig.cmake.in"
        "${CMAKE_CURRENT_BINARY_DIR}/JANAConfig.cmake"
        INSTALL_DESTINATION "lib/cmake/JANA"
)

# Generate JANAConfigVersion.cmake
write_basic_package_version_file(
        JANAConfigVersion.cmake
        VERSION ${PACKAGE_VERSION}
        COMPATIBILITY AnyNewerVersion
)

# Install JANAConfig.cmake and JANAConfigVersion.cmake
install(FILES "${CMAKE_CURRENT_BINARY_DIR}/JANAConfig.cmake" DESTINATION "lib/cmake/JANA")
install(FILES "${CMAKE_CURRENT_BINARY_DIR}/JANAConfigVersion.cmake" DESTINATION "lib/cmake/JANA")


# Export targets so that outside projects can use them


# Install "public" header files

file(GLOB jana_headers "*.h*")
file(GLOB jana_engine_headers "Engine/*.h*")
file(GLOB jana_services_headers "Services/*.h*")
file(GLOB jana_status_headers "Status/*.h*")
file(GLOB jana_streaming_headers "Streaming/*.h*")
file(GLOB jana_utils_headers "Utils/*.h*")
file(GLOB jana_calibs_headers "Calibrations/*.h*")
file(GLOB jana_cli_headers "CLI/*.h*")
file(GLOB jana_compat_headers "Compatibility/*.h*")
file(GLOB jana_podio_headers "Podio/*.h*")

install(FILES ${jana_headers} DESTINATION include/JANA)
install(FILES ${jana_engine_headers} DESTINATION include/JANA/Engine)
install(FILES ${jana_services_headers} DESTINATION include/JANA/Services)
install(FILES ${jana_status_headers} DESTINATION include/JANA/Status)
install(FILES ${jana_streaming_headers} DESTINATION include/JANA/Streaming)
install(FILES ${jana_utils_headers} DESTINATION include/JANA/Utils)
install(FILES ${jana_calibs_headers} DESTINATION include/JANA/Calibrations)
install(FILES ${jana_cli_headers} DESTINATION include/JANA/CLI)
install(FILES ${jana_compat_headers} DESTINATION include/JANA/Compatibility)

if (${USE_PODIO})
    install(FILES ${jana_podio_headers} DESTINATION include/JANA/Podio)
endif()






