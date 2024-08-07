
file(GLOB EventGroupExample_SOURCES "*.c*")
file(GLOB EventGroupExample_HEADERS "*.h*")

add_jana_plugin(EventGroupExample 
    SOURCES ${EventGroupExample_SOURCES} 
    PUBLIC_HEADER ${EventGroupExample_HEADERS})

add_test(NAME jana-example-eventgroup-tests 
    COMMAND jana -Pplugins=EventGroupExample)

set_tests_properties(jana-example-eventgroup-tests PROPERTIES DISABLED TRUE)
