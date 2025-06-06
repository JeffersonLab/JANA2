


if(USE_CUDA)
    #add_jana_plugin(podio_subevent_splitter_merger_cuda)
    #target_link_libraries(podio_subevent_splitter_merger_cuda PUBLIC podio_datamodel)
else()
    message(STATUS "Skipping examples/tutorial_with_podio_datamodel/18_subevent_splitter_merger_cuda because USE_CUDA=Off")
endif()


