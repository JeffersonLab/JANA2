
if (${USE_ROOT} AND ${USE_ZEROMQ})

    find_package(ZeroMQ REQUIRED)
    find_package(ROOT REQUIRED)

    file(GLOB InteractiveStreamingExample_SOURCES "*.c*")
    file(GLOB InteractiveStreamingExample_HEADERS "*.h*")

    add_jana_plugin(InteractiveStreamingExample 
        SOURCES ${InteractiveStreamingExample_SOURCES}
        PUBLIC_HEADER ${InteractiveStreamingExample_HEADERS})

    target_include_directories(InteractiveStreamingExample PUBLIC ${ZeroMQ_INCLUDE_DIRS})
    target_link_libraries(InteractiveStreamingExample PUBLIC ${ZeroMQ_LIBRARIES})

else()

    message(STATUS "Skipping examples/InteractiveStreamingExample because USE_ROOT=Off or USE_ZEROMQ=Off")

endif()
