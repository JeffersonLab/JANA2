

if(USE_ROOT)
    #add_jana_plugin(lw_root_file_reader)
else()
    message(STATUS "Skipping examples/tutorial_with_lightweight_datamodel/09_root_file_reader because USE_ROOT=Off")
endif()
