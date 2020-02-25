# JExample 7 plugin

set(EventGroupExample_SOURCES SHARED
        BlockingGroupedEventSource.h
        EventGroupExamplePlugin.cc
        GroupedEventProcessor.h
        GroupedEventSource.h
        TridasEvent.h
)

#find_package(JANA)
add_library(EventGroupExample_plugin SHARED ${EventGroupExample_SOURCES})

target_include_directories(EventGroupExample_plugin PUBLIC ${JANA_INCLUDE_DIRS})
target_link_libraries(EventGroupExample_plugin jana2 ${JANA_LIBRARIES})
set_target_properties(EventGroupExample_plugin PROPERTIES PREFIX "" SUFFIX ".so")

install(TARGETS EventGroupExample_plugin DESTINATION ${CMAKE_INSTALL_PREFIX}/plugins)


