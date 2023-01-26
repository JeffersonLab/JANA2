
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
    SubeventTests.cc
    JAutoactivableTests.cc
    JTablePrinterTests.cc
    JMultiFactoryTests.cc
    )

if (${USE_PODIO})
    list(APPEND TEST_SOURCES
            PodioTests.cc
            )
endif()

add_executable(janatests ${TEST_SOURCES})
find_package(Threads REQUIRED)
target_include_directories(janatests PUBLIC .)
target_link_libraries(janatests jana2)

if (${USE_PODIO})
    # Pull in the data model from examples/PodioExample.
    # We don't want to have two separate toy data models in the JANA codebase
    target_link_libraries(janatests DataModelLib DataModelDict)
endif()

install(TARGETS janatests DESTINATION bin)
install(FILES catch.hpp DESTINATION include/external)
