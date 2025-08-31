
#add_jana_plugin(podio_clustering_algorithm)

#target_link_libraries(podio_clustering_algorithm PUBLIC podio_datamodel)


# `libpodio_protocluster_facs` is a library that includes our protoclustering algorithm
# and one or more factories that control access to it. We keep this separate from the
# plugin itself because there may be multiple plugins that will want to use these factories
# in different contexts.
#
# Note that the `add_jana_library` macro also creates a `podio_protoclusterizer_facs_tests`
# executable target which is linked with the catch2 testing framework. This test suite will
# be automatically registered with CTest.

add_jana_library(podio_protocluster_facs
    SOURCES
        Protocluster_algorithm.cc
        Protocluster_factory.cc
        Protocluster_factory_epic.cc
    PUBLIC_HEADER
        Protocluster_algorithm.h
        Protocluster_factory.h
        Protocluster_factory_epic.h
    TESTS
        Protocluster_tests.cc
)

# `add_jana_library` will create a target for us, but we still need to link it against
# its various dependencies by ourselves.

target_link_libraries(podio_protocluster_facs PUBLIC jana2_tutorial_podio_datamodel)


# `podio_protoclusterizer` is the plugin that "pulls in" the factories from podio_protocluster_facs
# in order to set up a simple runnable example. A more realistic plugin would include all of
# the factories in the clustering chain for one particular subdetector.

add_jana_plugin(podio_protoclusterizer SOURCES protoclusterizer_plugin.cc)
target_link_libraries(podio_protoclusterizer PUBLIC podio_protocluster_facs)


# TODO: Add CTest integration tests
