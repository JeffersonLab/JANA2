

add_jana_plugin(DstExample)

add_test(
    NAME jana-example-dst-tests 
    COMMAND $<TARGET_FILE:jana> -Pplugins=DstExample)
