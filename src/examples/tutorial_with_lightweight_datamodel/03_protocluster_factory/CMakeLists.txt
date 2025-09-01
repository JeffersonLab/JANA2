
# `liblw_protocluster_facs` is a library that includes our protoclustering algorithm
# and one or more factories that control access to it. We keep this separate from the
# plugin itself because there may be multiple plugins that will want to use these factories
# in different contexts.
#
# Note that the `add_jana_library` macro also creates a `lw_protoclusterizer_facs_tests`
# executable target which is linked with the catch2 testing framework. This test suite will
# be automatically registered with CTest.

add_jana_library(lw_protocluster_common
    SOURCES
        Protocluster_algorithm.cc
        Protocluster_factory.cc
        Protocluster_factory_gluex.cc
        Protocluster_factory_epic.cc
    PUBLIC_HEADER
        Protocluster_algorithm.h
        Protocluster_factory.h
        Protocluster_factory_gluex.h
        Protocluster_factory_epic.h
    TESTS
        Protocluster_tests.cc
)

# `add_jana_library` will create a target for us, but we still need to link it against
# its various dependencies by ourselves.

target_link_libraries(lw_protocluster_common PUBLIC lw_datamodel)


# `lw_protoclusterizer` is the plugin that "pulls in" the factories from lw_protocluster_facs
# in order to set up a simple runnable example. A more realistic plugin would include all of
# the factories in the clustering chain for one particular subdetector.

add_jana_plugin(lw_protocluster SOURCES protocluster_plugin.cc)
target_link_libraries(lw_protocluster PUBLIC lw_protocluster_common)


# TODO: Add CTest integration tests
