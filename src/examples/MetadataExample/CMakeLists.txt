

add_jana_plugin(MetadataExample)

add_test(NAME jana-example-metadata-tests 
    COMMAND jana -Pplugins=MetadataExample)

set_tests_properties(jana-example-metadata-tests 
    PROPERTIES DISABLED TRUE)
