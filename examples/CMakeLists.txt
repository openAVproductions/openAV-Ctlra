# Detect platform
EXECUTE_PROCESS( COMMAND uname -m COMMAND tr -d '\n' OUTPUT_VARIABLE ARCHITECTURE)
message( STATUS "Architecture:  ${ARCHITECTURE}" )

# Find depend libraries
find_package(PkgConfig)

pkg_check_modules(CAIRO cairo REQUIRED)
include_directories( ${CAIRO_INCLUDE_DIRS}  )
link_directories   ( ${CAIRO_LIBRARY_DIRS}  )

pkg_check_modules(ALSA alsa REQUIRED)
include_directories( ${ALSA_INCLUDE_DIRS}  )
link_directories   ( ${ALSA_LIBRARY_DIRS}  )

pkg_check_modules(JACK jack REQUIRED)
include_directories( ${JACK_INCLUDE_DIRS}  )
link_directories   ( ${JACK_LIBRARY_DIRS}  )

pkg_check_modules(SNDFILE sndfile REQUIRED)
include_directories( ${SNDFILE_INCLUDE_DIRS}  )
link_directories   ( ${SNDFILE_LIBRARY_DIRS}  )

# needed for setting icon in WM
pkg_check_modules(X11 x11 REQUIRED)
include_directories( ${X11_INCLUDE_DIRS}  )
link_directories   ( ${X11_LIBRARY_DIRS}  )

IF(RELEASE_BUILD)
  SET(CMAKE_CXX_FLAGS " -g -Wall -Wno-unused-variable ")
  SET(CMAKE_C_FLAGS " -std=c99 -g -Wall -W -Wno-unused-variable ")
else()
  SET(CMAKE_C_FLAGS " -std=c99 ")
ENDIF(RELEASE_BUILD)

if( ${ARCHITECTURE} STREQUAL "x86_64" )
  #set(CMAKE_C_FLAGS   ${CMAKE_C_FLAGS}"-msse2" )
  #set(CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS}"-msse2" )
else()
  # Can set i686 / ARM stuff here
endif()


if(BUILD_ASAN)
  set(CMAKE_C_FLAGS   "${CMAKE_C_FLAGS}   -O1 -g -fsanitize=address -fno-omit-frame-pointer")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O1 -g -fsanitize=address -fno-omit-frame-pointer")
endif()

include_directories( "../ctlra/" )

cmake_policy(SET CMP0015 OLD)
link_directories   ( "../ctlra/" )

#####################################
### Simple
#####################################
FILE(GLOB src_simple simple/*.c )
add_executable (simple ${src_simple} )
target_link_libraries (simple ctlra)
target_link_libraries( simple ${USB_LIBRARIES}        )
target_link_libraries( simple ${ALSA_LIBRARIES}       )

#####################################
### Vegas Mode
#####################################
include_directories( "../ctlra/devices/*" )
FILE(GLOB src_vegas vegas_mode/*.c )
add_executable (vegas_mode ${src_vegas} )
target_link_libraries (vegas_mode ctlra)
target_link_libraries( vegas_mode ${USB_LIBRARIES}        )
target_link_libraries( vegas_mode ${JACK_LIBRARIES}       )
target_link_libraries( vegas_mode ${ALSA_LIBRARIES}       )
target_link_libraries( vegas_mode ${CAIRO_LIBRARIES}      )
target_link_libraries( vegas_mode ${SNDFILE_LIBRARIES}    )

IF(BUILD_TCC_SCRIPTING)
  #####################################
  ### TCC Scripting 
  #####################################
  FILE(GLOB src_tcc tcc_script/*.c )
  add_executable (tcc_script ${src_tcc} )
  target_link_libraries (tcc_script ctlra)
  target_link_libraries( tcc_script ${USB_LIBRARIES}        )
  target_link_libraries( tcc_script "-ltcc -ldl"            )
ENDIF()


#####################################
### Daemon
#####################################
# RtMIDI backends
ADD_DEFINITIONS( "-D__UNIX_JACK__"  )
ADD_DEFINITIONS( "-D__LINUX_ALSA__" )
FILE(GLOB src_daemon daemon/*.c )

add_executable (daemon ${src_daemon} )
add_dependencies(daemon ctlra)
target_link_libraries( daemon "-lctlra"               )
target_link_libraries( daemon "-lpthread"             )
target_link_libraries( daemon ${USB_LIBRARIES}        )
target_link_libraries( daemon ${ALSA_LIBRARIES}       )
target_link_libraries( daemon ${JACK_LIBRARIES}       )

FILE(GLOB src_seq sequencer/*.cpp sequencer/*.c )
add_executable (seq ${src_seq} )
add_dependencies(seq ctlra)
target_link_libraries( seq "-lctlra"               )
target_link_libraries( seq "-lpthread"             )
target_link_libraries( seq ${USB_LIBRARIES}        )
target_link_libraries( seq ${ALSA_LIBRARIES}       )
target_link_libraries( seq ${JACK_LIBRARIES}       )
