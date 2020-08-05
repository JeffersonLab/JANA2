
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
    JFactoryGenerator.h
    JFactorySet.cc
    JFactorySet.h
    JFactoryT.h
    JObject.h
    JSourceFactoryGenerator.h
    JCsvWriter.h
    JLogger.h

#    ArrowEngine/Arrow.h
#    ArrowEngine/ComponentArrows.h
#    ArrowEngine/ComponentArrows.cc
#    ArrowEngine/Worker.h
#    ArrowEngine/Worker.cc
#    ArrowEngine/Mailbox.h
#    ArrowEngine/Scheduler.h
#    ArrowEngine/Scheduler.cc
#    ArrowEngine/Topology.h
#    ArrowEngine/Topology.cc
#    ArrowEngine/ArrowProcessingController.cc
#    ArrowEngine/ArrowProcessingController.h

    Engine/JActivable.h
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
    Engine/JMailbox.h
    Engine/JScheduler.cc
    Engine/JScheduler.h
    Engine/JSubeventArrow.cc
    Engine/JSubeventArrow.h
    Engine/JWorker.h
    Engine/JWorker.cc
    Engine/JWorkerMetrics.h

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

    Calibrations/JCalibration.cc
    Calibrations/JCalibration.h
    Calibrations/JCalibrationCCDB.h
    Calibrations/JCalibrationFile.cc
    Calibrations/JCalibrationFile.h
    Calibrations/JCalibrationGenerator.h
    Calibrations/JCalibrationGeneratorCCDB.h

    CLI/JVersion.h
    CLI/JBenchmarker.cc
    CLI/JBenchmarker.h
    CLI/JSignalHandler.h

    Compatibility/JStreamLog.cc
    Compatibility/JStreamLog.h
    Compatibility/JStreamLogBuffer.cc
    Compatibility/JStreamLogBuffer.h
    Compatibility/JStatusBits.h
    Compatibility/jerror.h
    )


add_library(jana2 STATIC ${JANA2_SOURCES})

find_package(Threads REQUIRED)
set(THREADS_PREFER_PTHREAD_FLAG ON)
target_link_libraries(jana2 ${CMAKE_DL_LIBS} Threads::Threads)
set_target_properties(jana2 PROPERTIES PREFIX "lib" OUTPUT_NAME "JANA")

# Install library
install(TARGETS jana2 EXPORT jana2_targets DESTINATION lib)

# Install JANATargets.cmake
install(EXPORT jana2_targets FILE JANATargets.cmake NAMESPACE JANA:: DESTINATION lib/cmake/JANA)

# Generate JANAConfig.cmake, pointing it to JANATargets
include(CMakePackageConfigHelpers)
configure_package_config_file(
        "JANAConfig.cmake.in"
        "${CMAKE_CURRENT_BINARY_DIR}/JANAConfig.cmake"
        INSTALL_DESTINATION "lib/cmake/JANA"
)

# Generate JANAConfigVesrion.cmake
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

install(FILES ${jana_headers} DESTINATION include/JANA)
install(FILES ${jana_engine_headers} DESTINATION include/JANA/Engine)
install(FILES ${jana_services_headers} DESTINATION include/JANA/Services)
install(FILES ${jana_status_headers} DESTINATION include/JANA/Status)
install(FILES ${jana_streaming_headers} DESTINATION include/JANA/Streaming)
install(FILES ${jana_utils_headers} DESTINATION include/JANA/Utils)
install(FILES ${jana_calibs_headers} DESTINATION include/JANA/Calibrations)
install(FILES ${jana_cli_headers} DESTINATION include/JANA/CLI)
install(FILES ${jana_compat_headers} DESTINATION include/JANA/Compatibility)




