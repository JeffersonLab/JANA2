# streamDet plugin

add_library(streamDet SHARED
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

find_package(ZeroMQ REQUIRED)
find_package(CZMQ REQUIRED)
find_package(ROOT REQUIRED)

if ((ZeroMQ_FOUND AND CZMQ_FOUND) AND ROOT_FOUND)

    target_include_directories(streamDet PUBLIC ${ROOT_INCLUDE_DIRS} ${ZEROMQ_INCLUDE_DIRS} ${CMZQ_INCLUDE_DIRS})
    target_link_libraries(streamDet jana2 ${ZEROMQ_LIBRARIES} ${CZMQ_LIBRARIES} ${ROOT_LIBRARIES})
    set_target_properties(streamDet PROPERTIES PREFIX "" SUFFIX ".so")

else()

    message(WARNING "Unable to find ZeroMQ/CZMQ/ROOT! Skipping compilation of streamDet.so")

endif()


