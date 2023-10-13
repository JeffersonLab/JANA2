
set (UnitTestingExample_SOURCES
        SimpleClusterFactoryTests.cc
    )

add_executable(UnitTestingExample ${UnitTestingExample_SOURCES})

# Our copy of catch.hpp lives in this weird place. Maybe we should move it to ${PROJECT_SOURCE_DIR}/src/external
target_include_directories(UnitTestingExample PRIVATE ${PROJECT_SOURCE_DIR}/src/programs/tests)

target_link_libraries(UnitTestingExample Tutorial_plugin jana2)
set_target_properties(UnitTestingExample PROPERTIES PREFIX "" OUTPUT_NAME "UnitTestingExample")
install(TARGETS UnitTestingExample DESTINATION programs)
