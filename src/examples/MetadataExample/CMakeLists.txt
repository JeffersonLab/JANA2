
file(GLOB MetadataExample_SOURCES "*.c*")
file(GLOB MetadataExample_HEADERS "*.h*")

add_jana_plugin(MetadataExample 
    SOURCES ${MetadataExample_SOURCES}
    PUBLIC_HEADER ${MetadataExample_HEADERS})

add_test(NAME jana-example-metadata-tests 
    COMMAND jana -Pplugins=MetadataExample)

set_tests_properties(jana-example-metadata-tests 
    PROPERTIES DISABLED TRUE)
