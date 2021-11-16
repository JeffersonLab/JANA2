
add_library(regressiontest SHARED JEventProcessor_regressiontest.cc JIntrospection.cc)
find_package(Threads REQUIRED)
target_link_libraries(regressiontest jana2)
target_link_libraries(regressiontest Threads::Threads)
set_target_properties(regressiontest PROPERTIES PREFIX "" OUTPUT_NAME "regressiontest" SUFFIX ".so")
install(TARGETS regressiontest DESTINATION plugins)
