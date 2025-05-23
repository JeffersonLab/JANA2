
if(USE_ROOT)
    #add_jana_plugin(lw_root_histogram_writer)
else()
    message(STATUS "Skipping examples/tutorial_with_lightweight_datamodel/07_root_histogram_writer because USE_ROOT=Off")
endif()

