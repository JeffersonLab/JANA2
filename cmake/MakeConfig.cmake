

include(CMakePackageConfigHelpers)
configure_package_config_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake/JANAConfig.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/cmake/JANAConfig.cmake"
    INSTALL_DESTINATION "lib/JANA/cmake"
)

write_basic_package_version_file(
    "${CMAKE_CURRENT_BINARY_DIR}/cmake/JANAConfigVersion.cmake"
    VERSION ${PACKAGE_VERSION}
    COMPATIBILITY AnyNewerVersion
)

install(EXPORT jana2_targets 
    FILE "JANATargets.cmake"
    NAMESPACE JANA:: 
    DESTINATION "lib/JANA/cmake")

install(FILES "${CMAKE_CURRENT_BINARY_DIR}/cmake/JANAConfig.cmake" 
    DESTINATION "lib/JANA/cmake")

install(FILES "${CMAKE_CURRENT_BINARY_DIR}/cmake/JANAConfigVersion.cmake" 
    DESTINATION "lib/JANA/cmake")

install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/cmake/AddJanaPlugin.cmake"
    DESTINATION "lib/JANA/cmake")



