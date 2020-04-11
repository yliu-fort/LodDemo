# Locate the tiff library
#
# This module defines the following variables:
#
# TIFF_LIBRARY the name of the library;
# TIFF_INCLUDE_DIR where to find glfw include files.
# TIFF_FOUND true if both the TIFF_LIBRARY and TIFF_INCLUDE_DIR have been found.
#
# To help locate the library and include file, you can define a
# variable called TIFF_ROOT which points to the root of the glfw library
# installation.
#
# default search dirs
# 
# Cmake file from: https://github.com/daw42/glslcookbook

set( _tiff_HEADER_SEARCH_DIRS
"/usr/include"
"/usr/local/include"
"${CMAKE_SOURCE_DIR}/external"
"C:/Program Files (x86)/glfw/include" )
set( _tiff_LIB_SEARCH_DIRS
"/usr/lib"
"/usr/local/lib"
"${CMAKE_SOURCE_DIR}/lib"
"C:/Program Files (x86)/glfw/lib-msvc110" )

# Check environment for root search directory
set( _tiff_ENV_ROOT $ENV{TIFF_ROOT} )
if( NOT TIFF_ROOT AND _tiff_ENV_ROOT )
	set(TIFF_ROOT ${_tiff_ENV_ROOT} )
endif()

# Put user specified location at beginning of search
if( TIFF_ROOT )
	list( INSERT _tiff_HEADER_SEARCH_DIRS 0 "${TIFF_ROOT}/include" )
	list( INSERT _tiff_LIB_SEARCH_DIRS 0 "${TIFF_ROOT}/lib" )
endif()

# Search for the header
FIND_PATH(TIFF_INCLUDE_DIR "TIFF/tiff.h"
PATHS ${_tiff_HEADER_SEARCH_DIRS} )

# Search for the library
FIND_LIBRARY(TIFF_LIBRARY NAMES tiff
PATHS ${_tiff_LIB_SEARCH_DIRS} )
INCLUDE(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(TIFF DEFAULT_MSG
TIFF_LIBRARY TIFF_INCLUDE_DIR)
