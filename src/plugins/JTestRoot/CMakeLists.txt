
if(${USE_ROOT})
	find_package(ROOT REQUIRED)
	include(${ROOT_USE_FILE})

	# Generate ROOT dictionaries for each of our ROOT object types
    # The foreach statement below should end with a list of classes
    # that dictionaries should be created from. Headers for each class
    # will be determined by adding a ".h" suffix to them. The dictionary
    # PCM filenames will also be derived from this so they can be
    # installed in the proper directory.
    # n.b. it would be great if we could install the *.pcm files with
    # a GLOB, but they do not exist when cmake is run so we have to guess
    # the names.
	include_directories(${CMAKE_CURRENT_SOURCE_DIR})
    set(my_pcms)
    foreach( CLASS Hit Cluster )
        ROOT_GENERATE_DICTIONARY(G__${CLASS} ${CLASS}.h)
        list(APPEND my_pcms ${CMAKE_CURRENT_BINARY_DIR}/lib${CLASS}_rdict.pcm )
    endforeach()

	set (JTestRoot_PLUGIN_SOURCES
			JTestRoot.cc
			JTestRootEventSource.cc
			JTestRootEventSource.h
			JFactory_Cluster.cc
			JFactory_Cluster.h
			JTestRootProcessor.cc
			JTestRootProcessor.h
			Hit.h
			Cluster.h
			G__Hit.cxx
			G__Cluster.cxx
		)

	add_library(JTestRoot_plugin SHARED ${JTestRoot_PLUGIN_SOURCES})
	target_compile_definitions(JTestRoot_plugin PUBLIC HAVE_ROOT)
	target_include_directories(JTestRoot_plugin PUBLIC ${JANA_INCLUDE_DIR} ${ROOT_INCLUDE_DIRS})
	target_link_libraries(JTestRoot_plugin jana2)
	target_link_libraries(JTestRoot_plugin ${ROOT_LIBRARIES})
	set_target_properties(JTestRoot_plugin PROPERTIES PREFIX "" OUTPUT_NAME "JTestRoot" SUFFIX ".so")
	install(TARGETS JTestRoot_plugin DESTINATION plugins)

    message(STATUS "Installing ROOT PCM files: ${my_pcms}")
    install(FILES ${my_pcms} DESTINATION plugins)
        
    file(GLOB my_headers "*.h*")
    install(FILES ${my_headers} DESTINATION include/JTestRoot)

else()
	message(STATUS "Skipping plugins/JTestRoot because USE_ROOT=Off")

endif() # ROOT_FOUND
