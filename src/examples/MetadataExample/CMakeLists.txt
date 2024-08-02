

set (MetadataExample_PLUGIN_SOURCES
		MetadataExample.cc
		MetadataAggregator.cc
		MetadataAggregator.h
		Track.h
		TrackSmearingFactory.cc
		TrackSmearingFactory.h
		RandomTrackSource.cc
		RandomTrackSource.h
	)

add_library(MetadataExample_plugin SHARED ${MetadataExample_PLUGIN_SOURCES})

target_include_directories(MetadataExample_plugin PUBLIC ${JANA_INCLUDE_DIR})
target_link_libraries(MetadataExample_plugin jana2)
set_target_properties(MetadataExample_plugin PROPERTIES PREFIX "" OUTPUT_NAME "MetadataExample" SUFFIX ".so")
install(TARGETS MetadataExample_plugin DESTINATION lib/JANA/plugins)

