

if (USE_CUDA)
	find_package(CUDA REQUIRED)
	enable_language(CUDA)

    set(SubeventCUDAExample_SOURCES 
        SubeventCUDAExample.cu)

	add_executable(SubeventCUDAExample ${SubeventCUDAExample_SOURCES})

	target_link_libraries(SubeventCUDAExample jana2)

	set_target_properties(SubeventCUDAExample PROPERTIES 
        PREFIX "" 
        OUTPUT_NAME "SubeventCUDAExample"
		CUDA_ARCHITECTURES "75;80")

	install(TARGETS SubeventCUDAExample DESTINATION bin)
    add_test(NAME jana-example-subevent-cuda-tests
        COMMAND SubeventCUDAExample)

else()
	message(STATUS "Skipping examples/SubeventCUDAExample because USE_CUDA=Off")

endif()





