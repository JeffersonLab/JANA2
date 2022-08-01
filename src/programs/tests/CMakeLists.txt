
set(TEST_SOURCES
    catch.hpp
    ArrowActivationTests.cc
    JObjectTests.cc
    JServiceLocatorTests.cc
    JServiceLocatorTests.h
    QueueTests.cc
    TopologyTests.cc
    SchedulerTests.cc
    JEventTests.cc
    JEventTests.h
    PerformanceTests.cc
    PerformanceTests.h
    ExactlyOnceTests.cc
    ExactlyOnceTests.h
    TerminationTests.cc
    TerminationTests.h
    UserExceptionTests.cc
    UserExceptionTests.h
    JEventGroupTests.cc
    JFactoryTests.h
    JFactoryTests.cc
    NEventNSkipTests.cc
    JEventGetAllTests.cc
    JParameterManagerTests.cc
    JStatusBitsTests.cc
    TimeoutTests.cc
    ScaleTests.cc
    BarrierEventTests.cc
    BarrierEventTests.h
    GetObjectsTests.cc
    JCallGraphRecorderTests.cc
    JEventProcessorSequentialTests.cc
    JFactoryDefTagsTests.cc
    )

add_executable(janatests ${TEST_SOURCES})
find_package(Threads REQUIRED)
target_include_directories(janatests PUBLIC .)
target_link_libraries(janatests jana2 Threads::Threads)

install(TARGETS janatests DESTINATION bin)
install(FILES catch.hpp DESTINATION include/external)
