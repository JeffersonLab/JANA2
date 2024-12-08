
add_library(VendoredMD5 OBJECT md5.c)
target_include_directories(VendoredMD5 PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>)



