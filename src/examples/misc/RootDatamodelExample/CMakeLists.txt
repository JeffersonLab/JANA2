
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

    set (RootDatamodelExample_HEADERS
        Hit.h 
        Cluster.h 
        JFactory_Cluster.h
		JTestRootEventSource.h
		JTestRootProcessor.h)

    set (RootDatamodelExample_SOURCES
        RootDatamodelExample.cc
        JTestRootEventSource.cc
        JFactory_Cluster.cc
        JTestRootProcessor.cc
        G__Hit.cxx
        G__Cluster.cxx)

    add_jana_plugin(RootDatamodelExample 
        SOURCES ${RootDatamodelExample_SOURCES}
        PUBLIC_HEADER ${RootDatamodelExample_HEADERS})

    target_include_directories(RootDatamodelExample PUBLIC ${ROOT_INCLUDE_DIRS})
    target_link_libraries(RootDatamodelExample PUBLIC ${ROOT_LIBRARIES})

    message(STATUS "Installing ROOT PCM files: ${my_pcms}")
    install(FILES ${my_pcms} DESTINATION lib/JANA/plugins)

    add_test(NAME jana-example-rootdatamodel-tests
        COMMAND jana -Pplugins=RootDatamodelExample -Pjana:nevents=10)
        
    set_tests_properties(jana-example-rootdatamodel-tests
        PROPERTIES DISABLED TRUE)

else()
    message(STATUS "Skipping plugins/RootDatamodelExample because USE_ROOT=Off")

endif() # ROOT_FOUND
