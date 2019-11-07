# JExample 7 plugin

set(JEXAMPLE7_SOURCES SHARED
    ZmqMain.cc
    ZmqTransport.h
    ReadoutMessageAuto.h
    AHit.h
    AHitAnomalyDetector.h
    AHitParser.h
    )

find_package(ZeroMQ)
find_package(CZMQ)

if (ZEROMQ_FOUND AND CZMQ_FOUND)

    add_library(JExample7 SHARED ${JEXAMPLE7_SOURCES})

else()

    add_library(JExample7 SHARED EXCLUDE_FROM_ALL ${JEXAMPLE7_SOURCES})
    message(WARNING "Skipping compilation of JExample7.so")

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

target_include_directories(JExample7 PUBLIC ${ZEROMQ_INCLUDE_DIRS} ${CMZQ_INCLUDE_DIRS})
target_link_libraries(JExample7 jana2 ${ZEROMQ_LIBRARIES} ${CZMQ_LIBRARIES})
set_target_properties(JExample7 PROPERTIES PREFIX "" SUFFIX ".so")
install(TARGETS JExample7 DESTINATION plugins)


