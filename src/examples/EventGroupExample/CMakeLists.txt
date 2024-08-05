
file(GLOB EventGroupExample_SOURCES "*.c*")
file(GLOB EventGroupExample_HEADERS "*.h*")

add_jana_plugin(EventGroupExample 
    SOURCES ${EventGroupExample_SOURCES} 
    PUBLIC_HEADER ${EventGroupExample_HEADERS})


