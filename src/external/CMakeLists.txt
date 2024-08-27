
add_library(VendoredCatch2 INTERFACE)

target_include_directories(VendoredCatch2 INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
    $<INSTALL_INTERFACE:include/JANA/external>
)

target_include_directories(VendoredTomlPlusPlus INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
    $<INSTALL_INTERFACE:include/JANA/external>
)

install(FILES catch.hpp toml.hpp
    DESTINATION include/JANA/external)

install(TARGETS VendoredCatch2
    EXPORT jana2_targets
    DESTINATION include/JANA/external)

install(TARGETS VendoredTomlPlusPlus
    EXPORT jana2_targets
    DESTINATION include/JANA/external)

