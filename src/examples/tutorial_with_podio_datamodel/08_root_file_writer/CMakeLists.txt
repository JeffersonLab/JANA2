
if(USE_ROOT)
    #add_jana_plugin(podio_root_file_writer)
    #target_link_libraries(podio_root_file_writer PUBLIC podio_datamodel podio_datamodel_dict podio::podioRootIO)
else()
    message(STATUS "Skipping examples/tutorial_with_podio_datamodel/08_root_file_writer because USE_ROOT=Off")
endif()

