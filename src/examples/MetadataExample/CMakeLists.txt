
file(GLOB MetadataExample_SOURCES "*.c*")
file(GLOB MetadataExample_HEADERS "*.h*")

add_jana_plugin(MetadataExample 
    SOURCES ${MetadataExample_SOURCES}
    PUBLIC_HEADER ${MetadataExample_HEADERS})


