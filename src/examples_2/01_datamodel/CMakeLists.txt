
# This is a "header-only" CMake target. Use it to include any of the headers
# via `target_link_libraries(SOMETHING PUBLIC JObjectDatamodel)`. It works both
# in-tree and externally.

add_library(JObjectExamples::Datamodel INTERFACE)

target_include_directories(JObjectExamples::Datamodel INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
    $<INSTALL_INTERFACE:include/JANA/JObjectExamples/Datamodel
)

file(GLOB JObjectExampleDatamodel_HEADERS "*.h")

install(FILES ${JObjectExampleDatamodel_HEADERS}
    DESTINATION include/JANA/JObjectExamples/Datamodel)


