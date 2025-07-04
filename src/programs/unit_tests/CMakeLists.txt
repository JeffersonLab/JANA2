
set(TEST_SOURCES
    
    Topology/JArrowTests.cc
    Topology/JPoolTests.cc
    Topology/MultiLevelTopologyTests.cc
    Topology/QueueTests.cc
    #Topology/SubeventTests.cc

    Components/BarrierEventTests.cc
    Components/JObjectTests.cc
    Components/ExactlyOnceTests.cc
    Components/GetObjectsTests.cc
    Components/NEventNSkipTests.cc
    Components/JComponentTests.cc
    Components/JDatabundleTests.cc
    Components/JEventGetAllTests.cc
    Components/JEventProcessorTests.cc
    Components/JEventSourceTests.cc
    Components/JEventTests.cc
    Components/JFactoryDefTagsTests.cc
    Components/JFactoryTests.cc
    Components/JFactoryGeneratorTests.cc
    Components/JHasInputsTests.cc
    Components/JMultiFactoryTests.cc
    Components/JServiceTests.cc
    Components/UnfoldTests.cc
    Components/UserExceptionTests.cc

    Services/JServiceLocatorTests.cc
    Services/JParameterManagerTests.cc
    Services/JWiringServiceTests.cc

    Engine/ScaleTests.cc
    Engine/TerminationTests.cc
    Engine/TimeoutTests.cc
    Engine/JExecutionEngineTests.cc

    Utils/JAutoActivatorTests.cc
    Utils/JEventGroupTests.cc
    Utils/JTablePrinterTests.cc
    Utils/JStatusBitsTests.cc
    Utils/JCallGraphRecorderTests.cc
    Utils/JLoggerTests.cc
    )

if (${USE_PODIO})
    list(APPEND TEST_SOURCES Components/PodioTests.cc)
endif()

add_jana_test(jana-unit-tests SOURCES ${TEST_SOURCES})

if (${USE_PODIO})
    # Pull in the data model from examples/PodioExample.
    # We don't want to have multiple separate toy data models in the JANA codebase
    target_link_libraries(jana-unit-tests PRIVATE PodioDatamodel PodioDatamodelDict)
endif()


