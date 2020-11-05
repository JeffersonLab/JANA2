# streamDet plugin

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

find_package(ZeroMQ)
find_package(ROOT)

if (ZeroMQ_FOUND AND ROOT_FOUND)

    add_library(streamDet SHARED ${STREAMDET_SOURCES})
    target_include_directories(streamDet PUBLIC ${ROOT_INCLUDE_DIRS} ${ZeroMQ_INCLUDE_DIRS})
    target_link_libraries(streamDet jana2 ${ZeroMQ_LIBRARIES} ${ROOT_LIBRARIES})
    set_target_properties(streamDet PROPERTIES PREFIX "" SUFFIX ".so")
    install(TARGETS streamDet DESTINATION plugins)

else()

    message(WARNING "Skipping target `streamDet` due to missing ZeroMQ or ROOT dependencies")

endif()


