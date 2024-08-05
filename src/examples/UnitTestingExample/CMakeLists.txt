
set (UnitTestingExample_SOURCES
        SimpleClusterFactoryTests.cc
    )

add_executable(UnitTestingExample ${UnitTestingExample_SOURCES})

target_link_libraries(UnitTestingExample Tutorial VendoredCatch2)
set_target_properties(UnitTestingExample PROPERTIES PREFIX "" OUTPUT_NAME "UnitTestingExample")
install(TARGETS UnitTestingExample DESTINATION bin)
