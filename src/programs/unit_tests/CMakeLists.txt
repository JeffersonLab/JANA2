
set(TEST_SOURCES
    
    Topology/ArrowTests.cc
    Topology/JPoolTests.cc
    Topology/MultiLevelTopologyTests.cc
    Topology/QueueTests.cc
    Topology/SubeventTests.cc
    Topology/TopologyTests.cc

    Components/BarrierEventTests.cc
    Components/JObjectTests.cc
    Components/ExactlyOnceTests.cc
    Components/GetObjectsTests.cc
    Components/NEventNSkipTests.cc
    Components/JComponentTests.cc
    Components/JEventGetAllTests.cc
    Components/JEventProcessorTests.cc
    Components/JEventProcessorSequentialTests.cc
    Components/JEventSourceTests.cc
    Components/JEventTests.cc
    Components/JFactoryDefTagsTests.cc
    Components/JFactoryTests.cc
    Components/JMultiFactoryTests.cc
    Components/UnfoldTests.cc
    Components/UserExceptionTests.cc

    Services/JServiceLocatorTests.cc
    Services/JParameterManagerTests.cc

    Engine/ArrowActivationTests.cc
    Engine/ScaleTests.cc
    Engine/SchedulerTests.cc
    Engine/TerminationTests.cc
    Engine/TimeoutTests.cc

    Utils/JAutoactivableTests.cc
    Utils/JEventGroupTests.cc
    Utils/JTablePrinterTests.cc
    Utils/JStatusBitsTests.cc
    Utils/JCallGraphRecorderTests.cc

    )

if (${USE_PODIO})
    list(APPEND TEST_SOURCES
        Components/PodioTests.cc
        )
endif()

add_executable(jana-unit-tests ${TEST_SOURCES})
find_package(Threads REQUIRED)
target_include_directories(jana-unit-tests PUBLIC .)
target_link_libraries(jana-unit-tests jana2)

if (${USE_PODIO})
    # Pull in the data model from examples/PodioExample.
    # We don't want to have two separate toy data models in the JANA codebase
    target_link_libraries(jana-unit-tests PodioExampleDatamodel PodioExampleDatamodelDict)
endif()

install(TARGETS jana-unit-tests DESTINATION bin)
install(FILES ../../external/catch.hpp DESTINATION include/external)
