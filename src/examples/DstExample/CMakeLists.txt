
file(GLOB DSTExample_SOURCES "*.c*")
file(GLOB DSTExample_HEADERS "*.h*")

add_jana_plugin(DstExample 
    SOURCES ${DSTExample_SOURCES} 
    PUBLIC_HEADER ${DSTExample_HEADERS})


