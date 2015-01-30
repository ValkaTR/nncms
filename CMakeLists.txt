cmake_minimum_required(VERSION 2.6)
if(${CMAKE_VERSION} STREQUAL "2.8.2")
  # bug http://vtk.org/Bug/view.php?id=11020
  message( WARNING "CMake/CPack version 2.8.2 will not create working .deb packages!")
endif(${CMAKE_VERSION} STREQUAL "2.8.2")

# This can be read from ${PROJECT_NAME} after project() is called
project(nncms C)

set(VERSION_EXTRA "" CACHE STRING "Stuff to append to version string")

# Also remember to set PROTOCOL_VERSION in clientserver.h when releasing
set(VERSION_MAJOR 0)
set(VERSION_MINOR 4)
set(VERSION_PATCH dev-20120707)
if(VERSION_EXTRA)
	set(VERSION_PATCH ${VERSION_PATCH}-${VERSION_EXTRA})
endif()
set(VERSION_STRING "${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_PATCH}")

MESSAGE(STATUS "*** Will build version ${VERSION_STRING} ***")

# Configuration options

set(WARN_ALL 1 CACHE BOOL "Enable -Wall for Release build")

if(NOT CMAKE_BUILD_TYPE)
	# Default to release
	set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type: Debug or Release" FORCE)
endif()

# Included stuff
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/Modules/")


ADD_DEFINITIONS(-std=gnu99)


#
# Subdirectories
# Be sure to add all relevant definitions above this
#

add_subdirectory(src)
