
cmake_minimum_required (VERSION 2.6)

project (CTLRA)

set(CTLRA_VERSION_MAJOR "0")
set(CTLRA_VERSION_MINOR "0")
set(CTLRA_VERSION_PATCH "1")

set(CTLRA_VERSION "${CTLRA_VERSION_MAJOR}.${CTLRA_VERSION_MINOR}.${CTLRA_VERSION_PATCH}")

#option(BUILD_TESTS "Build test version" OFF)

#set(CMAKE_VERBOSE_MAKEFILE on)

execute_process(COMMAND git log --oneline -n1
    OUTPUT_VARIABLE _output OUTPUT_STRIP_TRAILING_WHITESPACE)
    file(WRITE src/version.hxx "#define GIT_VERSION \"${_output}\"")


add_subdirectory (ctlra)
add_subdirectory (examples)
