

add_executable(SubeventExample SubeventExample.cc)
target_link_libraries(SubeventExample jana2)
set_target_properties(SubeventExample PROPERTIES PREFIX "" OUTPUT_NAME "SubeventExample")
install(TARGETS SubeventExample DESTINATION bin)

add_test(NAME jana-example-subevent-tests
    COMMAND SubeventExample)

set_tests_properties(jana-example-subevent-tests PROPERTIES DISABLED TRUE)

