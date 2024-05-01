
set (UnitTestingExample_SOURCES
        SimpleClusterFactoryTests.cc
    )

add_executable(UnitTestingExample ${UnitTestingExample_SOURCES})

# Need to be able to find catch.hpp
target_include_directories(UnitTestingExample PRIVATE ${CMAKE_SOURCE_DIR}/src/external)

target_link_libraries(UnitTestingExample Tutorial_plugin jana2)
set_target_properties(UnitTestingExample PROPERTIES PREFIX "" OUTPUT_NAME "UnitTestingExample")
install(TARGETS UnitTestingExample DESTINATION programs)
