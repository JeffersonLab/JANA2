#
# Top-level CMakeLists.txt for JANA2 python interfaces
#

add_subdirectory(externals/pybind11-2.5.0)


# Note that the common directory contains source common to
# both the plugins and modules directories. We do not,
# however build it as a library here. Rather, it is
# compiled by each of those separately (so it is compiled
# multiple times).

add_subdirectory(plugins/janapy)
add_subdirectory(modules/jana)
