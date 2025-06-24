
if(USE_ROOT)
    #add_jana_plugin(podio_root_event_viewer)
    #target_link_libraries(podio_root_event_viewer PUBLIC podio_datamodel)
else()
    message(STATUS "Skipping examples/tutorial_with_podio_datamodel/10_root_event_viewer because USE_ROOT=Off")
endif()



