

set (BlockExample_PLUGIN_SOURCES
		main.cc
		BlockExampleProcessor.cc
		BlockExampleProcessor.h
	)

add_library(BlockExample_plugin SHARED ${BlockExample_PLUGIN_SOURCES})

find_package(JANA REQUIRED)
target_include_directories(BlockExample_plugin PUBLIC ${JANA_INCLUDE_DIR})
target_link_libraries(BlockExample_plugin ${JANA_LIBRARY})
set_target_properties(BlockExample_plugin PROPERTIES PREFIX "" OUTPUT_NAME "BlockExample" SUFFIX ".so")
install(TARGETS BlockExample_plugin DESTINATION plugins)


set (BlockExample_TEST_SOURCES
	tests/BlockExampleProcessorTest.cc
)

add_executable(block_example_tests ${BlockExample_TEST_SOURCES})
target_include_directories(block_example_tests PUBLIC . ${JANA_INCLUDE_DIR})
target_link_libraries(block_example_tests BlockExample_plugin ${JANA_LIBRARY})

