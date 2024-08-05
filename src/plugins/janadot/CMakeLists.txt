
file(GLOB JANADOT_HEADERS "*.h*")
add_jana_plugin(janadot SOURCES JEventProcessorJANADOT.cc PUBLIC_HEADER ${JANADOT_HEADERS})
find_package(Threads REQUIRED)
target_link_libraries(janadot PUBLIC Threads::Threads)

