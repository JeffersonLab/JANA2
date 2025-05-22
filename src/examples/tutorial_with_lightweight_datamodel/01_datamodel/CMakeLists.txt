
# This is a "header-only" CMake target. Use it to include any of the headers
# via `target_link_libraries(SOMETHING PUBLIC JObjectDatamodel)`. It works both
# in-tree and externally.

add_library(jana2_tut_lw_01_datamodel INTERFACE)

target_include_directories(jana2_tut_lw_01_datamodel INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
    # $<INSTALL_INTERFACE:include/JANA/TutorialWithLightweightDatamodel/Datamodel
)

file(GLOB JObjectExampleDatamodel_HEADERS "*.h")

# install(FILES ${JObjectExampleDatamodel_HEADERS}
#    DESTINATION include/JANA/TutorialWithLightweightDatamodel/Datamodel)


