
if(USE_ROOT)
    #add_jana_plugin(lw_root_event_viewer)
else()
    message(STATUS "Skipping examples/tutorial_with_lightweight_datamodel/10_root_event_viewer because USE_ROOT=Off")
endif()

