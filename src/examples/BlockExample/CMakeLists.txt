

set (BlockExample_SOURCES
		main.cc
		BlockExampleProcessor.cc
		BlockExampleProcessor.h
		BlockExampleSource.cc
		BlockExampleSource.h
	)

add_executable(BlockExample ${BlockExample_SOURCES})

#find_package(JANA REQUIRED)
#target_include_directories(BlockExample PUBLIC ${JANA_INCLUDE_DIR})
target_link_libraries(BlockExample jana2)
set_target_properties(BlockExample PROPERTIES PREFIX "" OUTPUT_NAME "BlockExample" SUFFIX ".so")
install(TARGETS BlockExample DESTINATION plugins)


#set (BlockExample_TEST_SOURCES
#	tests/BlockExampleProcessorTest.cc
#)

#add_executable(block_example_tests ${BlockExample_TEST_SOURCES})
#target_include_directories(block_example_tests PUBLIC . ${JANA_INCLUDE_DIR})
#target_link_libraries(block_example_tests BlockExample_plugin ${JANA_LIBRARY})

