
add_library(VendoredTomlPlusPlus INTERFACE)

target_include_directories(VendoredTomlPlusPlus INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
    $<INSTALL_INTERFACE:include/JANA/external/tomlplusplus>
)

install(FILES toml.hpp
    DESTINATION include/JANA/external/tomlplusplus)

install(TARGETS VendoredTomlPlusPlus
    EXPORT jana2_targets)

