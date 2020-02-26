

set (Tutorial_PLUGIN_SOURCES
		Tutorial.cc
		TutorialProcessor.cc
		TutorialProcessor.h
		RandomSource.cc
		RandomSource.h
		Hit.h
		Cluster.h
		SimpleClusterFactory.cc
		SimpleClusterFactory.h
	)

add_library(Tutorial_plugin SHARED ${Tutorial_PLUGIN_SOURCES})

target_link_libraries(Tutorial_plugin jana2)
set_target_properties(Tutorial_plugin PROPERTIES PREFIX "" OUTPUT_NAME "Tutorial" SUFFIX ".so")
install(TARGETS Tutorial_plugin DESTINATION plugins)

