
find_package(ZeroMQ)
if (ZeroMQ_FOUND)

    set (janacontrol_PLUGIN_SOURCES
            janacontrol.cc
            JControlZMQ.cc
            JControlZMQ.h
            JControlEventProcessor.cc
            JControlEventProcessor.h
        )

    add_library(janacontrol_plugin SHARED ${janacontrol_PLUGIN_SOURCES})

    find_package(Threads REQUIRED)

	target_include_directories(janacontrol_plugin PUBLIC ${JANA_INCLUDE_DIR} ${ZeroMQ_INCLUDE_DIRS})
    target_link_libraries(janacontrol_plugin jana2 Threads::Threads ${ZeroMQ_LIBRARY})
	set_target_properties(janacontrol_plugin PROPERTIES PREFIX "" OUTPUT_NAME "janacontrol" SUFFIX ".so")
	install(TARGETS janacontrol_plugin DESTINATION plugins)

else()

	message(WARNING "Skipping target `janacontrol` due to missing ZeroMQ")

endif()

