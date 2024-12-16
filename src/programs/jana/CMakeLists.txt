

add_executable(jana jana.cc )

find_package(Threads REQUIRED)
target_link_libraries(jana jana2_shared_lib)
target_link_options(jana PRIVATE -rdynamic)
install(TARGETS jana DESTINATION bin)

