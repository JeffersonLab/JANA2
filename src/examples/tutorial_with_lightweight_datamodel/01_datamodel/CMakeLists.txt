
add_library(lw_datamodel INTERFACE)

target_include_directories(lw_datamodel INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/jana2_tutorial_lightweight/01_datamodel>
)

file(GLOB lw_datamodel_headers "*.h")

install(FILES ${lw_datamodel_headers}
        DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/jana2_tutorial_lightweight/01_datamodel)

install(TARGETS lw_datamodel
        EXPORT jana2_targets)


