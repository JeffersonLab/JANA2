
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

    Engine/JActivable.h
    Engine/JArrow.h
    Engine/JArrowMetrics.h
    Engine/JArrowPerfSummary.cc
    Engine/JArrowPerfSummary.h
    Engine/JArrowProcessingController.cc
    Engine/JArrowProcessingController.h
    Engine/JArrowTopology.cc
    Engine/JArrowTopology.h
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
    Services/JParameter.cc
    Services/JParameter.h
    Services/JParameterManager.cc
    Services/JParameterManager.h
    Services/JPluginLoader.cc
    Services/JPluginLoader.h
    Services/JProcessingController.h
    Services/JServiceLocator.cc
    Services/JServiceLocator.h

    Status/JComponentSummary.h
    Status/JComponentSummary.cc
    Status/JPerfMetrics.cc
    Status/JPerfMetrics.h
    Status/JPerfSummary.h
    Status/JStatus.cc
    Status/JStatus.h
    Status/JVersion.h

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
    )


find_package(Threads REQUIRED)

set(THREADS_PREFER_PTHREAD_FLAG ON)
add_library(jana2 STATIC ${JANA2_SOURCES})
target_link_libraries(jana2 ${CMAKE_DL_LIBS} Threads::Threads)

# Install library
install(TARGETS jana2 DESTINATION lib)

# Install "public" header files

file(GLOB jana_headers "*.h*")
file(GLOB jana_engine_headers "Engine/*.h*")
file(GLOB jana_services_headers "Services/*.h*")
file(GLOB jana_status_headers "Status/*.h*")
file(GLOB jana_streaming_headers "Streaming/*.h*")
file(GLOB jana_utils_headers "Utils/*.h*")

install(FILES ${jana_headers} DESTINATION include)
install(FILES ${jana_engine_headers} DESTINATION include/Engine)
install(FILES ${jana_services_headers} DESTINATION include/Services)
install(FILES ${jana_status_headers} DESTINATION include/Status)
install(FILES ${jana_streaming_headers} DESTINATION include/Streaming)
install(FILES ${jana_utils_headers} DESTINATION include/Utils)




