
add_library(regressiontest SHARED JEventProcessor_regressiontest.cc JInspector.cc)
find_package(Threads REQUIRED)
target_link_libraries(regressiontest jana2)
target_link_libraries(regressiontest Threads::Threads)
set_target_properties(regressiontest PROPERTIES PREFIX "" OUTPUT_NAME "regressiontest" SUFFIX ".so")
install(TARGETS regressiontest DESTINATION plugins)

file(GLOB my_headers "*.h*")
install(FILES ${my_headers} DESTINATION include/regressiontest)
