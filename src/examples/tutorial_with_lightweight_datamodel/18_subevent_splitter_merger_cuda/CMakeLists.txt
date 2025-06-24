
if(USE_CUDA)
    #add_jana_plugin(lw_subevent_splitter_merger_cuda)
else()
    message(STATUS "Skipping examples/tutorial_with_lightweight_datamodel/18_subevent_splitter_merger_cuda because USE_CUDA=Off")
endif()
