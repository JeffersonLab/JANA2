

# This is a "header-only" CMake target. Use it to include any of the headers
# via `target_link_libraries(SOMETHING PUBLIC JObjectDatamodel)`. It works both
# in-tree and externally.

add_library(Examples::JObjectDatamodel INTERFACE)

target_include_directories(Examples::JObjectDatamodel INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
    $<INSTALL_INTERFACE:include/JANA/JObjectDatamodel
)

file(GLOB JObjectDatamodel_HEADERS "*.h")

install(FILES ${JObjectDatamodel_HEADERS}
    DESTINATION include/JANA/JObjectDatamodel)

install(TARGETS Examples::JObjectDatamodel
    EXPORT jana2_targets)

