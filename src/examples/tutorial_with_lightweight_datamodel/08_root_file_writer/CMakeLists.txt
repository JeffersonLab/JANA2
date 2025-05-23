
if(USE_ROOT)
    #add_jana_plugin(lw_root_file_writer)
else()
    message(STATUS "Skipping examples/tutorial_with_lightweight_datamodel/08_root_file_writer because USE_ROOT=Off")
endif()
