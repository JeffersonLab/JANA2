
if (${USE_ZEROMQ})

    set (janacontrol_PLUGIN_SOURCES
            janacontrol.cc
            JControlZMQ.cc
            JControlZMQ.h
            JControlEventProcessor.cc
            JControlEventProcessor.h
        )

    find_package(ZeroMQ REQUIRED)
    find_package(Threads REQUIRED)

    add_library(janacontrol_plugin SHARED ${janacontrol_PLUGIN_SOURCES})

    target_include_directories(janacontrol_plugin PUBLIC ${JANA_INCLUDE_DIR} ${ZeroMQ_INCLUDE_DIRS})
    target_link_libraries(janacontrol_plugin jana2 Threads::Threads ${ZeroMQ_LIBRARY})
    set_target_properties(janacontrol_plugin PROPERTIES PREFIX "" OUTPUT_NAME "janacontrol" SUFFIX ".so")
    install(TARGETS janacontrol_plugin DESTINATION plugins)

else()
    message(STATUS "Skipping plugins/janacontrol because USE_ZEROMQ=Off")

endif()

