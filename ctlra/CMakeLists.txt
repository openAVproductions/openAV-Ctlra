# Detect platform
EXECUTE_PROCESS( COMMAND uname -m COMMAND tr -d '\n' OUTPUT_VARIABLE ARCHITECTURE)
message( STATUS "Architecture:  ${ARCHITECTURE}" )

# Find depend libraries
find_package(PkgConfig)

pkg_check_modules(USB libusb-1.0 REQUIRED)
include_directories( ${USB_INCLUDE_DIRS}  )
link_directories   ( ${USB_LIBRARY_DIRS}  )

pkg_check_modules(ALSA alsa REQUIRED)
include_directories( ${ALSA_INCLUDE_DIRS}  )
link_directories   ( ${ALSA_LIBRARY_DIRS}  )

IF(RELEASE_BUILD)
  SET(CMAKE_CXX_FLAGS " -g -Wall -Wno-unused-variable ")
  SET(CMAKE_C_FLAGS " -g -Wall -W -Wno-unused-variable ")
ENDIF(RELEASE_BUILD)

SET(CMAKE_C_FLAGS " -g -fPIC -Wall -Wno-unused-variable ")

if( ${ARCHITECTURE} STREQUAL "x86_64" )
  set(CMAKE_C_FLAGS   ${CMAKE_C_FLAGS}"-msse2" )
  set(CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS}"-msse2" )
else()
  # Can set i686 / ARM stuff here
endif()

# Add all Ctlra library files
FILE(GLOB sources *.c  devices/*.c)

include_directories( "." )

ADD_LIBRARY(ctlra STATIC ${sources} )

target_include_directories(ctlra PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

target_link_libraries( ctlra PRIVATE ${USB_LIBRARIES}        )
target_link_libraries( ctlra PRIVATE ${ALSA_LIBRARIES}       )
target_link_libraries( ctlra PRIVATE "-lasound"              )
target_link_libraries(ctlra PRIVATE "-lasound")
target_link_libraries( ctlra PRIVATE "-lpthread"             )


install(FILES ctlra.h       DESTINATION include)
install(FILES event.h       DESTINATION include)
install(TARGETS ctlra       DESTINATION lib)
