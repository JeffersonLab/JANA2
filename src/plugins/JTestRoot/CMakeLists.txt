
if(${USE_ROOT})
	find_package(ROOT REQUIRED)
	include(${ROOT_USE_FILE})

	# Generate ROOT dictionaries for each of our ROOT object types
	include_directories(${CMAKE_CURRENT_SOURCE_DIR})
	ROOT_GENERATE_DICTIONARY(G__Hit Hit.h)
	ROOT_GENERATE_DICTIONARY(G__Cluster Cluster.h)

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
	target_link_libraries(JTestRoot_plugin ${JANA_LIBRARY} ${ROOT_LIBRARIES})
	set_target_properties(JTestRoot_plugin PROPERTIES PREFIX "" OUTPUT_NAME "JTestRoot" SUFFIX ".so")
	install(TARGETS JTestRoot_plugin DESTINATION plugins)

else()
	message(WARNING "Skipping compilation of JTestRoot.so because USE_ROOT=Off")

endif() # ROOT_FOUND
