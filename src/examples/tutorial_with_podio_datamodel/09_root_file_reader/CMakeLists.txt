
if(USE_ROOT)
    #add_jana_plugin(podio_root_file_reader)
    #target_link_libraries(podio_root_file_reader PUBLIC podio_datamodel podio_datamodel_dict podio::podioRootIO)
else()
    message(STATUS "Skipping examples/tutorial_with_podio_datamodel/09_root_file_reader because USE_ROOT=Off")
endif()


