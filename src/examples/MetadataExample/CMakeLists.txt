

add_jana_plugin(MetadataExample)

add_test(NAME jana-example-metadata-tests 
    COMMAND jana -Pplugins=MetadataExample)

