

set (Tutorial_SOURCES
    Tutorial.cc
    RandomSource.cc
    TutorialProcessor.cc
    SimpleClusterFactory.cc)

set (Tutorial_HEADERS
    TutorialProcessor.h
    RandomSource.h
    Hit.h
    Cluster.h
    SimpleClusterFactory.h)

add_jana_plugin(Tutorial 
    SOURCES ${Tutorial_SOURCES}
    PUBLIC_HEADER ${Tutorial_HEADERS})

add_test(NAME jana-example-tutorial-tests
    COMMAND jana -Pplugins=Tutorial)

set_tests_properties(jana-example-tutorial-tests PROPERTIES DISABLED TRUE)

