
add_library(JExample1 SHARED JExample1.cc)
target_link_libraries(JExample1 jana2)
set_target_properties(JExample1 PROPERTIES PREFIX "" SUFFIX ".so")
install(TARGETS JExample1 DESTINATION plugins)
