
if(USE_ROOT)
    #add_jana_plugin(podio_root_histogram_writer)
    #target_link_libraries(podio_root_histogram_writer PUBLIC podio_datamodel)
else()
    message(STATUS "Skipping examples/tutorial_with_podio_datamodel/07_root_histogram_writer because USE_ROOT=Off")
endif()

