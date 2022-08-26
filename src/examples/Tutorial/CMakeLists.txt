

set (Tutorial_PLUGIN_SOURCES
        QuickTutorial.cc
        TutorialProcessor.cc
        TutorialProcessor.h
        RandomSource.cc
        RandomSource.h
        Hit.h
        Cluster.h
	Cluster_factory_Simple.cc
	Cluster_factory_Simple.h
    )

add_library(Tutorial_plugin SHARED ${Tutorial_PLUGIN_SOURCES})

target_link_libraries(Tutorial_plugin jana2)
set_target_properties(Tutorial_plugin PROPERTIES PREFIX "" OUTPUT_NAME "Tutorial" SUFFIX ".so")
install(TARGETS Tutorial_plugin DESTINATION plugins)
