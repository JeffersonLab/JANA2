
set(TEST_SOURCES
    catch.hpp
    ActivableTests.cc
    JServiceLocatorTests.cc
    JServiceLocatorTests.h
    QueueTests.cc
    TopologyTests.cc
    SchedulerTests.cc
    JEventTests.cc
    JEventTests.h
    PerformanceTests.cc
    PerformanceTests.h
    TestTopology.cc
    TestTopology.h
    ExactlyOnceTests.cc
    ExactlyOnceTests.h
    TerminationTests.cc
    TerminationTests.h
    UserExceptionTests.cc
    UserExceptionTests.h
    JEventGroupTests.cc
    JFactoryTests.h
    JFactoryTests.cc
    )

add_executable(tests ${TEST_SOURCES})
find_package(Threads REQUIRED)
target_include_directories(tests PUBLIC .)
target_link_libraries(tests jana2 Threads::Threads)
install(TARGETS tests DESTINATION bin)
