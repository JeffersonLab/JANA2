cmake_minimum_required(VERSION 3.13)
project(jana2)

set(CMAKE_CXX_STANDARD 14)

# Enable -fPIC for all targets
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# JANA,playground,tests all need threads
set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

include_directories(src/lib)
include_directories(src/lib/JANA)
include_directories(src/plugins/JTest)
include_directories(src/programs/jana)
include_directories(src/programs/tests)

add_library(jana2 STATIC
        src/lib/JANA/JApplication.cc
        src/lib/JANA/JApplication.h
        src/lib/JANA/JCpuInfo.cc
        src/lib/JANA/JCpuInfo.h
        src/lib/JANA/JEvent.cc
        src/lib/JANA/JEvent.h
        src/lib/JANA/JEventProcessor.h
        src/lib/JANA/JEventSource.cc
        src/lib/JANA/JEventSource.h
        src/lib/JANA/JEventSourceGenerator.h
        src/lib/JANA/JEventSourceGeneratorT.h
        src/lib/JANA/JEventSourceManager.cc
        src/lib/JANA/JEventSourceManager.h
        src/lib/JANA/JException.cc
        src/lib/JANA/JException.h
        src/lib/JANA/JFactory.h
        src/lib/JANA/JFactoryGenerator.h
        src/lib/JANA/JFactorySet.cc
        src/lib/JANA/JFactorySet.h
        src/lib/JANA/JFactoryT.h
        src/lib/JANA/JFunctions.cc
        src/lib/JANA/JFunctions.h
        src/lib/JANA/JLogger.h
        src/lib/JANA/JObject.cc
        src/lib/JANA/JObject.h
        src/lib/JANA/JParameter.cc
        src/lib/JANA/JParameter.h
        src/lib/JANA/JParameterManager.cc
        src/lib/JANA/JParameterManager.h
        src/lib/JANA/JQueue.h
        src/lib/JANA/JQueueSet.cc
        src/lib/JANA/JQueueSet.h
        src/lib/JANA/JQueueSimple.cc
        src/lib/JANA/JQueueSimple.h
        src/lib/JANA/JQueueWithBarriers.cc
        src/lib/JANA/JQueueWithBarriers.h
        src/lib/JANA/JQueueWithLock.cc
        src/lib/JANA/JQueueWithLock.h
        src/lib/JANA/JResettable.h
        src/lib/JANA/JResourcePool.h
        src/lib/JANA/JSignalHandler.h
        src/lib/JANA/JSourceFactoryGenerator.h
        src/lib/JANA/JStatus.cc
        src/lib/JANA/JStatus.h
        src/lib/JANA/JTask.h
        src/lib/JANA/JThread.cc
        src/lib/JANA/JThread.h
        src/lib/JANA/JThreadManager.cc
        src/lib/JANA/JThreadManager.h
        src/lib/JANA/JTypeInfo.h
        src/lib/JANA/JVersion.h
        src/lib/JANA/JEventProcessorArrow.cc
        src/lib/JANA/JEventProcessorArrow.h
        src/lib/JANA/JEventSourceArrow.cc
        src/lib/JANA/JEventSourceArrow.h
        src/lib/JANA/JScheduler.cc
        src/lib/JANA/JPerfUtils.cc
        src/lib/JANA/JServiceLocator.cc
        src/lib/JANA/JWorker.h
        src/lib/JANA/JWorker.cc
        src/lib/JANA/JSubeventArrow.cc
        src/lib/JANA/JSubeventArrow.h
        src/lib/JANA/JProcessingController.h
        src/lib/JANA/JArrowMetrics.h
        src/lib/JANA/JWorkerMetrics.h
        src/lib/JANA/JTopologyBuilder.cc
        src/lib/JANA/JTopologyBuilder.h
        src/lib/JANA/JPluginLoader.cc
        src/lib/JANA/JPluginLoader.h
        src/lib/JANA/JLegacyProcessingController.cc
        src/lib/JANA/JLegacyProcessingController.h
        src/lib/JANA/JProcessingTopology.cpp
        src/lib/JANA/JProcessingTopology.h
        src/lib/JANA/JArrowProcessingController.cc
        src/lib/JANA/JArrowProcessingController.h
        src/lib/JANA/JPerfMetrics.cc
        src/lib/JANA/JPerfMetrics.h
        src/lib/JANA/JPerfSummary.h
        src/lib/JANA/JArrowPerfSummary.cc
        src/lib/JANA/JArrowPerfSummary.h
        src/lib/JANA/JEventPool.h
        src/lib/JANA/JProcessorMapping.h
        src/lib/JANA/JProcessorMapping.cc
        src/lib/JANA/Streaming/JEventBuilder.h
        src/lib/JANA/Streaming/JStreamingEventSource.h
        src/lib/JANA/Streaming/JTransport.h
        src/lib/JANA/Streaming/JMessage.h
        src/lib/JANA/Streaming/JWindow.h
        src/lib/JANA/Streaming/JMergeWindow.h
        src/lib/JANA/JComponentSummary.h
        src/lib/JANA/JComponentSummary.cc
        src/lib/JANA/JCsvWriter.h
        )
target_link_libraries(jana2 ${CMAKE_DL_LIBS})
target_link_libraries(jana2 Threads::Threads)


add_executable(jana
        src/programs/jana/jana.cc
        src/programs/jana/JBenchmarker.h
        src/programs/jana/JBenchmarker.cc
        src/programs/jana/MyProcessor.cc
        src/programs/jana/MyProcessor.h
        )

target_link_libraries(jana jana2)
target_link_libraries(jana Threads::Threads)


add_executable(tests
        src/programs/tests/catch.hpp
        src/programs/tests/ActivableTests.cc
        src/programs/tests/JServiceLocatorTests.cc
        src/programs/tests/JServiceLocatorTests.h
        src/programs/tests/QueueTests.cc
        src/programs/tests/TopologyTests.cc
        src/programs/tests/SchedulerTests.cc
        src/programs/tests/JEventTests.cc
        src/programs/tests/JEventTests.h
        src/programs/tests/PerformanceTests.cc
        src/programs/tests/PerformanceTests.h
        src/programs/tests/TestTopology.cc
        src/programs/tests/TestTopology.h
        src/programs/tests/ExactlyOnceTests.cc
        src/programs/tests/ExactlyOnceTests.h
        src/programs/tests/TerminationTests.cc
        src/programs/tests/TerminationTests.h
        src/programs/tests/UserExceptionTests.cc
        src/programs/tests/UserExceptionTests.h
        )
target_link_libraries(tests jana2)
target_link_libraries(tests Threads::Threads)

add_executable(playground
        src/programs/playground/playground.cc
        src/programs/playground/factory_gens_iter_1.cc
        src/programs/playground/Factory.cc
        src/programs/playground/Factory.h
        src/programs/playground/JContext.cc
        src/programs/playground/JContext.h
        src/programs/playground/FactoryGenerator.h
        src/programs/playground/Factory.cc)
target_link_libraries(playground jana2)
target_link_libraries(playground Threads::Threads)

add_library(jtest SHARED
        src/lib/JANA/JThread.cc  #for THREAD_ID
        src/lib/JANA/JPerfUtils.cc
        src/plugins/JTest/JTestMain.cc
        src/plugins/JTest/JTestDataObjects.h
        src/plugins/JTest/JTestParser.h
        src/plugins/JTest/JTestDisentangler.h
        src/plugins/JTest/JTestTracker.h
        src/plugins/JTest/JTestPlotter.h
        src/plugins/JTest/JTestFactoryGenerator.h
        src/plugins/JTest/JTestCalibrationService.h)

target_link_libraries(jtest jana2)
target_link_libraries(jtest Threads::Threads)
set_target_properties(jtest PROPERTIES PREFIX "" SUFFIX ".so")

add_library(janapy SHARED
        src/plugins/janapy/janapy.cc
)
find_package(PythonInterp 3 REQUIRED)
find_package(PythonLibs 3 REQUIRED)
include_directories(${PYTHON_INCLUDE_DIRS})
target_link_libraries(janapy ${PYTHON_LIBRARIES})
target_link_libraries(janapy jana2)
set_target_properties(janapy PROPERTIES PREFIX "")


add_library(JExample7 SHARED
        src/examples/JExample7/ZmqMain.cc
        src/examples/JExample7/ZmqTransport.h
        src/examples/JExample7/ReadoutMessageAuto.h
        src/examples/JExample7/AHit.h
        src/examples/JExample7/AHitAnomalyDetector.h
        src/examples/JExample7/AHitParser.h
        )
find_package(ZeroMQ)
target_link_libraries(JExample7 jana2)
target_link_libraries(JExample7 libzmq-static)
set_target_properties(JExample7 PROPERTIES PREFIX "" SUFFIX ".so")

add_library(toyDet SHARED
        src/plugins/toyDet/JEventProcessor_toyDet.cc
        src/plugins/toyDet/JEventProcessor_toyDet.h
        src/plugins/toyDet/JEventSource_toyDet.cc
        src/plugins/toyDet/JEventSource_toyDet.h
        src/plugins/toyDet/JFactoryGenerator_toyDet.h
        src/plugins/toyDet/rawSamples.cc
        src/plugins/toyDet/rawSamples.h
        src/plugins/toyDet/toyDet.cc
        src/plugins/toyDet/ZmqTransport.h
        src/plugins/toyDet/INDRAMessage.h
        src/plugins/toyDet/JFactory_rawSamples.h
        )

find_package(ROOT)
include_directories(${ROOT_INCLUDE_DIRS})
target_link_libraries(toyDet jana2)
target_link_libraries(toyDet libzmq-static)
target_link_libraries(toyDet ${ROOT_LIBRARIES})
set_target_properties(toyDet PROPERTIES PREFIX "" SUFFIX ".so")



