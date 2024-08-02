
if(${USE_ROOT})
	find_package(ROOT REQUIRED)

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

    set (RootDatamodelExample_PLUGIN_SOURCES
            RootDatamodelExample.cc
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

    add_library(RootDatamodelExample_plugin SHARED ${RootDatamodelExample_PLUGIN_SOURCES})
    target_include_directories(RootDatamodelExample_plugin PUBLIC ${JANA_INCLUDE_DIR} ${ROOT_INCLUDE_DIRS})
    target_link_libraries(RootDatamodelExample_plugin jana2)
    target_link_libraries(RootDatamodelExample_plugin ${ROOT_LIBRARIES})
    set_target_properties(RootDatamodelExample_plugin PROPERTIES PREFIX "" OUTPUT_NAME "JTestRoot" SUFFIX ".so")
    install(TARGETS RootDatamodelExample_plugin DESTINATION lib/JANA/plugins)

    message(STATUS "Installing ROOT PCM files: ${my_pcms}")
    install(FILES ${my_pcms} DESTINATION lib/JANA/plugins)
        
    file(GLOB my_headers "*.h*")
    install(FILES ${my_headers} DESTINATION include/JANA/plugins/RootDatamodelExample)

else()
    message(STATUS "Skipping plugins/RootDatamodelExample because USE_ROOT=Off")

endif() # ROOT_FOUND
