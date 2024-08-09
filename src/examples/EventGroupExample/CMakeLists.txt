

add_jana_plugin(EventGroupExample)

add_test(NAME jana-example-eventgroup-tests 
    COMMAND jana -Pplugins=EventGroupExample)

set_tests_properties(jana-example-eventgroup-tests PROPERTIES DISABLED TRUE)
