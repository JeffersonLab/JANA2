#
# Top-level CMakeLists.txt for JANA2 python interfaces
#
#
# Note, the python interface requires pybind11. A version
# is bundled with JANA, but the user may also specify an
# external version. The easiest way to get the latests
# version of pybind11 and build JANA against it is:
#
#  python -m venv
#  source venv/bin/activate
#  pip install pybind11
#  export pybind11_ROOT=${PWD}/venv/lib/python3.*/site-packages/pybind11
#  
# Then, just specify -DUSE_PYTHON=1 to the cmake configure
# command.
#
# This also supports setting the USE_BUNDLED_PYBIND11 cmake
# variable to force using the bundled or external pybind11.
# If USE_BUNDLED_PYBIND11 is not set then an external package
# is first tried using find_package(pybind11) and if that fails
# it falls back to the bundled version.
#
# JANA2 developer's note:
#   To get a new version of pybind11 to bundle do something
#   like this:
#      cd src/python/externals
#      wget https://github.com/pybind/pybind11/archive/refs/tags/v2.10.3.tar.gz
#      tar xzf v2.10.3.tar.gz
#      git rm -r pybind11-2.6.1
#      git add pybind11-2.10.3
#

if (${USE_PYTHON})

    set(bundled_pybind11 externals/pybind11-2.10.3)

    # If the user has defined USE_BUNDLED_PYBIND11 then
    # exclusively try using what they indicated.
    #
    # If USE_BUNDLED_PYBIND11 is not set, then first
    # look in the environment with find_package() and if
    # not found there fall back to the bundled version.
    if(DEFINED USE_BUNDLED_PYBIND11)
        if( ${USE_BUNDLED_PYBIND11} )
            add_subdirectory(${bundled_pybind11})
            message(STATUS "Found pybind11: ${pybind11_INCLUDE_DIR}")
        else()
            find_package(pybind11 REQUIRED)
        endif()
    else()
        find_package(pybind11 QUIET)
        if(NOT ${pybind11_FOUND})
            message(STATUS "External pybind11 not found (set pybind11_ROOT envar. to use external). Using bundled version.")
            add_subdirectory(${bundled_pybind11})
            message(STATUS "Found pybind11: ${pybind11_INCLUDE_DIR}")
        endif()
    endif()

    # dump_cmake_variables(pybind11)

    # Note that the common directory contains source common to
    # both the plugins and modules directories. We do not,
    # however build it as a library here. Rather, it is
    # compiled by each of those separately (so it is compiled
    # multiple times).

    include_directories(${PROJECT_SOURCE_DIR}/src/plugins)  # needed to include janacontrol/janaJSON.h

    add_subdirectory(plugins/janapy)
    add_subdirectory(modules/jana)

else()
    message(STATUS "Skipping python/* because USE_PYTHON=Off")
endif()
