# JExample 7 plugin

set(example_eventgroups_SOURCES SHARED
        event_group_example.cc
)

#find_package(JANA)
add_library(example_eventgroups SHARED ${example_eventgroups_SOURCES})

target_include_directories(example_eventgroups PUBLIC ${JANA_INCLUDE_DIRS})
target_link_libraries(example_eventgroups jana2 ${JANA_LIBRARIES})
set_target_properties(example_eventgroups PROPERTIES PREFIX "" SUFFIX ".so")

install(TARGETS example_eventgroups DESTINATION ${JANA_HOME}/plugins)


