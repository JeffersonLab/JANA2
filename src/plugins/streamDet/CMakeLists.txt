
if (${USE_ROOT} AND ${USE_ZEROMQ})

    find_package(ZeroMQ REQUIRED)
    find_package(ROOT REQUIRED)

    set(STREAMDET_SOURCES
            MonitoringProcessor.cc
            MonitoringProcessor.h
            RootProcessor.cc
            RootProcessor.h
            DecodeDASSource.cc
            DecodeDASSource.h
            JFactoryGenerator_streamDet.h
            streamDet.cc
            ZmqTransport.h
            INDRAMessage.h
            ADCSample.h
            ADCSampleFactory.h
            )

    add_library(streamDet SHARED ${STREAMDET_SOURCES})
    target_include_directories(streamDet PUBLIC ${ROOT_INCLUDE_DIRS} ${ZeroMQ_INCLUDE_DIRS})
    target_link_libraries(streamDet jana2 ${ZeroMQ_LIBRARIES} ${ROOT_LIBRARIES})
    set_target_properties(streamDet PROPERTIES PREFIX "" SUFFIX ".so")
    install(TARGETS streamDet DESTINATION plugins)
else()
    message(STATUS "Skipping plugins/streamDet because USE_ROOT=Off or USE_ZEROMQ=Off")
endif()
