
find_package(ROOT REQUIRED)
if(${ROOT_FOUND})

include(${ROOT_USE_FILE})

set (JTestRoot_PLUGIN_SOURCES
		JTestRoot.cc
		JTestRootEventSource.cc
		JTestRootEventSource.h
		Hit.h
		Cluster.h
		JFactory_Cluster.cc
		JFactory_Cluster.h
		JTestRootProcessor.cc
		JTestRootProcessor.h
	)

add_library(JTestRoot_plugin SHARED ${JTestRoot_PLUGIN_SOURCES})
target_include_directories(JTestRoot_plugin PUBLIC ${JANA_INCLUDE_DIR} ${ROOT_INCLUDE_DIRS})
target_link_libraries(JTestRoot_plugin jana2)
target_link_libraries(JTestRoot_plugin ${JANA_LIBRARY} ${ROOT_LIBRARIES})
set_target_properties(JTestRoot_plugin PROPERTIES PREFIX "" OUTPUT_NAME "JTestRoot" SUFFIX ".so")
install(TARGETS JTestRoot_plugin DESTINATION plugins)

endif() # ROOT_FOUND