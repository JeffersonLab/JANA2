
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
   )

add_executable(tests ${TEST_SOURCES})

target_link_libraries(tests jana2 Threads::Threads)

# install(TARGETS tests DESTINATION bin)
# Want to install to JANA_HOME, not to /usr/local/...
# Also want to be able to find plugins without doing an install?