
cmake_minimum_required (VERSION 2.6)

project (CTLRA C)

set(CTLRA_VERSION_MAJOR "0")
set(CTLRA_VERSION_MINOR "0")
set(CTLRA_VERSION_PATCH "1")

set(CTLRA_VERSION "${CTLRA_VERSION_MAJOR}.${CTLRA_VERSION_MINOR}.${CTLRA_VERSION_PATCH}")

# The TCC scripting example requires libtcc-dev, which is not packaged by
# all distros, and requires some more manual moving of files to enable the
# script files to be found. It is not easy to do, so currently its disabled
option(BUILD_TCC_SCRIPTING "Build TCC scripting example"        OFF)
option(BUILD_ASAN          "Build with Clang Address Sanitizer" OFF)
option(MIDI_IO_ENABLED     "Build with MIDI I/O using RtMIDI"   ON)


execute_process(COMMAND git log --oneline -n1
    OUTPUT_VARIABLE _output OUTPUT_STRIP_TRAILING_WHITESPACE)
    file(WRITE ctlra/version.hxx "#define GIT_VERSION \"${_output}\"")


add_subdirectory (ctlra)
add_subdirectory (examples)
