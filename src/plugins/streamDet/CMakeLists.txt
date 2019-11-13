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

find_package(ZeroMQ REQUIRED)
find_package(CZMQ REQUIRED)
find_package(ROOT REQUIRED)

if ((ZEROMQ_FOUND AND CZMQ_FOUND) AND ROOT_FOUND)

    add_library(streamDet SHARED ${STREAMDET_SOURCES})

else()

    add_library(streamDet SHARED EXCLUDE_FROM_ALL ${STREAMDET_SOURCES})
    message(WARNING "Skipping compilation of streamDet.so")

    if (NOT ZEROMQ_FOUND)
        message(WARNING "Unable to find ZeroMQ")
    endif()

    if (NOT CZMQ_FOUND)
        message(WARNING "Unable to find CZMQ")
    endif()

    if (NOT ROOT_FOUND)
        message(WARNING "Unable to find ROOT")
    endif()

endif()

target_include_directories(streamDet PUBLIC ${ROOT_INCLUDE_DIRS} ${ZEROMQ_INCLUDE_DIRS} ${CMZQ_INCLUDE_DIRS})
target_link_libraries(streamDet jana2 ${ZEROMQ_LIBRARIES} ${CZMQ_LIBRARIES} ${ROOT_LIBRARIES})
set_target_properties(streamDet PROPERTIES PREFIX "" SUFFIX ".so")
install(TARGETS streamDet DESTINATION plugins)

