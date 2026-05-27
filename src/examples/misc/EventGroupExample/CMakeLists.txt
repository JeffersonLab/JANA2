

add_jana_plugin(EventGroupExample)

add_test(NAME jana-example-eventgroup-tests 
    COMMAND $<TARGET_FILE:jana> -Pplugins=EventGroupExample)

