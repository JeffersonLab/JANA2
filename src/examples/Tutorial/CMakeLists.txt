

add_jana_plugin(Tutorial)

add_test(NAME jana-example-tutorial-tests
    COMMAND jana -Pplugins=Tutorial)

set_tests_properties(jana-example-tutorial-tests PROPERTIES DISABLED TRUE)

