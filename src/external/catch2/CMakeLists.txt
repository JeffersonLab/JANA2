
add_library(VendoredCatch2 INTERFACE)

target_include_directories(VendoredCatch2 INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
    $<INSTALL_INTERFACE:include/JANA/external/catch2>
)

install(TARGETS VendoredCatch2
    EXPORT jana2_targets)

install(FILES catch.hpp
    DESTINATION include/JANA/external/catch2)


