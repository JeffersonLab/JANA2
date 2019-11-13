
add_library(JExample2 SHARED
        JEventProcessor_example2.h
        JEventSource_example2.cc
        JEventSource_example2.h
        JFactoryGenerator_example2.h
        JExample2.cc
        MyCluster.cc
        MyCluster.h
        MyEvent.h
        MyHit.h
        )

target_link_libraries(JExample2 jana2)
set_target_properties(JExample2 PROPERTIES PREFIX "" SUFFIX ".so")
install(TARGETS JExample2 DESTINATION plugins)

