#------------------------------------------------------------------------------#
# Distributed under the OSI-approved Apache License, Version 2.0.  See
# accompanying file Copyright.txt for details.
#------------------------------------------------------------------------------#
#
# FindWAVELET
# -----------
#
# Try to find the WAVELET library
#
# This module defines the following variables:
#
#   WAVELET_FOUND        - System has WAVELET
#   WAVELET_INCLUDE_DIRS - The WAVELET include directory
#   WAVELET_LIBRARIES    - Link these to use WAVELET
#
# and the following imported targets:
#   WAVELET::WAVELET - The WAVELET compression library target
#
# You can also set the following variable to help guide the search:
#   WAVELET_ROOT - The install prefix for WAVELET containing the
#              include and lib folders
#              Note: this can be set as a CMake variable or an
#                    environment variable.  If specified as a CMake
#                    variable, it will override any setting specified
#                    as an environment variable.

if(NOT WAVELET_FOUND)
  if((NOT WAVELET_ROOT) AND (NOT (ENV{WAVELET_ROOT} STREQUAL "")))
    set(WAVELET_ROOT "$ENV{WAVELET_ROOT}")
  endif()
  if(WAVELET_ROOT)
    set(WAVELET_INCLUDE_OPTS HINTS ${WAVELET_ROOT}/include NO_DEFAULT_PATHS)
    set(WAVELET_LIBRARY_OPTS
      HINTS ${WAVELET_ROOT}/lib ${WAVELET_ROOT}/lib64
      NO_DEFAULT_PATHS
    )
  endif()

  find_path(WAVELET_INCLUDE_DIR wavelib.h ${WAVELET_INCLUDE_OPTS})
  find_library(WAVELET_LIBRARY NAMES wavelib ${WAVELET_LIBRARY_OPTS})

  include(FindPackageHandleStandardArgs)
  find_package_handle_standard_args(Wavelet
    FOUND_VAR WAVELET_FOUND
    REQUIRED_VARS WAVELET_LIBRARY WAVELET_INCLUDE_DIR
  )
  if(WAVELET_FOUND)
    set(WAVELET_INCLUDE_DIRS ${WAVELET_INCLUDE_DIR})
    set(WAVELET_LIBRARIES ${WAVELET_LIBRARY})
    if(WAVELET_FOUND AND NOT TARGET Wavelet::Wavelet)
      add_library(Wavelet::Wavelet UNKNOWN IMPORTED)
      set_target_properties(Wavelet::Wavelet PROPERTIES
        IMPORTED_LOCATION             "${WAVELET_LIBRARY}"
        INTERFACE_LINK_LIBRARIES      "${WAVELET_LIBRARIES}"
        INTERFACE_INCLUDE_DIRECTORIES "${WAVELET_INCLUDE_DIR}"
      )
    endif()
  endif()
endif()
