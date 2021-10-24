
add_library(janadot SHARED JEventProcessorJANADOT.cc)
find_package(Threads REQUIRED)
target_link_libraries(janadot jana2)
target_link_libraries(janadot Threads::Threads)
set_target_properties(janadot PROPERTIES PREFIX "" OUTPUT_NAME "janadot" SUFFIX ".so")
install(TARGETS janadot DESTINATION plugins)
