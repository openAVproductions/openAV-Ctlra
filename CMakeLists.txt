
cmake_minimum_required (VERSION 2.6)

project (CTLRA C CXX)

set(CTLRA_VERSION_MAJOR "0")
set(CTLRA_VERSION_MINOR "0")
set(CTLRA_VERSION_PATCH "1")

set(CTLRA_VERSION "${CTLRA_VERSION_MAJOR}.${CTLRA_VERSION_MINOR}.${CTLRA_VERSION_PATCH}")

# The TCC scripting example requires libtcc-dev, which is not packaged by
# all distros, and requires some more manual moving of files to enable the
# script files to be found. It is not easy to do, so currently its disabled
option(BUILD_TCC_SCRIPTING "Build TCC scripting example"        OFF)
option(BUILD_ASAN          "Build with Clang Address Sanitizer" OFF)

add_subdirectory (ctlra)
add_subdirectory (examples)
