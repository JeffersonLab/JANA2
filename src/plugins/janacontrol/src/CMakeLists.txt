

set (janacontrol_PLUGIN_SOURCES
		janacontrol.cc
		JControlZMQ.cc
        JControlZMQ.h
	)

add_library(janacontrol_plugin SHARED ${janacontrol_PLUGIN_SOURCES})

find_package(JANA REQUIRED)
find_package(ZeroMQ)
if (ZEROMQ_FOUND)

	target_include_directories(janacontrol_plugin PUBLIC ${JANA_INCLUDE_DIR} ${ZEROMQ_INCLUDE_DIRS})
	target_link_libraries(janacontrol_plugin ${JANA_LIBRARY} ${ZEROMQ_LIBRARIES})
	set_target_properties(janacontrol_plugin PROPERTIES PREFIX "" OUTPUT_NAME "janacontrol" SUFFIX ".so")
	install(TARGETS janacontrol_plugin DESTINATION plugins)

else()

	message(WARNING "Skipping target `janacontrol` due to missing ZeroMQ, or CZMQ dependencies")

endif()

