

include(CMakePackageConfigHelpers)
configure_package_config_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake/JANAConfig.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/cmake/JANAConfig.cmake"
    INSTALL_DESTINATION "${CMAKE_INSTALL_LIBDIR}/JANA/cmake"
)

write_basic_package_version_file(
    "${CMAKE_CURRENT_BINARY_DIR}/cmake/JANAConfigVersion.cmake"
    VERSION ${PACKAGE_VERSION}
    COMPATIBILITY AnyNewerVersion
)

install(EXPORT jana2_targets 
    FILE "JANATargets.cmake"
    NAMESPACE JANA:: 
    DESTINATION "${CMAKE_INSTALL_LIBDIR}/JANA/cmake")

install(FILES "${CMAKE_CURRENT_BINARY_DIR}/cmake/JANAConfig.cmake" 
    DESTINATION "${CMAKE_INSTALL_LIBDIR}/JANA/cmake")

install(FILES "${CMAKE_CURRENT_BINARY_DIR}/cmake/JANAConfigVersion.cmake" 
    DESTINATION "${CMAKE_INSTALL_LIBDIR}/JANA/cmake")

install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/cmake/AddJanaPlugin.cmake"
    DESTINATION "${CMAKE_INSTALL_LIBDIR}/JANA/cmake")

install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/cmake/AddJanaLibrary.cmake"
    DESTINATION "${CMAKE_INSTALL_LIBDIR}/JANA/cmake")

install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/cmake/AddJanaTest.cmake"
    DESTINATION "${CMAKE_INSTALL_LIBDIR}/JANA/cmake")

