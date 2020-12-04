
find_package(ZEROMQ)
if (ZEROMQ_FOUND)

    set (janacontrol_PLUGIN_SOURCES
            janacontrol.cc
            JControlZMQ.cc
            JControlZMQ.h
            JControlEventProcessor.cc
            JControlEventProcessor.h
        )

    add_library(janacontrol_plugin SHARED ${janacontrol_PLUGIN_SOURCES})

    find_package(Threads REQUIRED)

    #---------------------------------------------------------------------------------------
    # ROOT is optional
    find_package(ROOT)
    if(${ROOT_FOUND})
        include(${ROOT_USE_FILE})
        target_compile_definitions(janacontrol_plugin PUBLIC HAVE_ROOT)
        target_include_directories(janacontrol_plugin PUBLIC ${ROOT_INCLUDE_DIRS})
        target_link_libraries(janacontrol_plugin ${ROOT_LIBRARIES})
    endif()
    #---------------------------------------------------------------------------------------

	target_include_directories(janacontrol_plugin PUBLIC ${JANA_INCLUDE_DIR} ${ZEROMQ_INCLUDE_DIRS})
    target_link_libraries(janacontrol_plugin jana2 Threads::Threads ${ZEROMQ_LIBRARIES})
	set_target_properties(janacontrol_plugin PROPERTIES PREFIX "" OUTPUT_NAME "janacontrol" SUFFIX ".so")
	install(TARGETS janacontrol_plugin DESTINATION plugins)

else()

	message(WARNING "Skipping target `janacontrol` due to missing ZeroMQ")

endif()

