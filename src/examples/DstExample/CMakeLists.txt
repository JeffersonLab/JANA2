

set (DstExample_PLUGIN_SOURCES
		DstExample.cc
		DstExampleProcessor.cc
		DstExampleProcessor.h
		DataObjects.h
	)

add_library(DstExample_plugin SHARED ${DstExample_PLUGIN_SOURCES})

target_include_directories(DstExample_plugin PUBLIC ${JANA_INCLUDE_DIR})
target_link_libraries(DstExample_plugin ${JANA_LIBRARY})
set_target_properties(DstExample_plugin PROPERTIES PREFIX "" OUTPUT_NAME "DstExample" SUFFIX ".so")
install(TARGETS DstExample_plugin DESTINATION plugins)

