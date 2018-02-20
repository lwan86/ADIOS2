#------------------------------------------------------------------------------#
# Distributed under the OSI-approved Apache License, Version 2.0.  See
# accompanying file Copyright.txt for details.
#------------------------------------------------------------------------------#
#
# FindLustre
# -----------
#
# Try to find the Lustre library
#
# This module defines the following variables:
#
#   Lustre_FOUND        - System has Lustre
#   Lustre_INCLUDE_DIRS - The Lustre include directory
#   Lustre_LIBRARIES    - Link these to use Lustre
#
# and the following imported targets:
#   Lustre::Lustre - The Lustre compression library target
#
# You can also set the following variable to help guide the search:
#   Lustre_ROOT - The install prefix for Lustre containing the
#              include and lib folders
#              Note: this can be set as a CMake variable or an
#                    environment variable.  If specified as a CMake
#                    variable, it will override any setting specified
#                    as an environment variable.

if(NOT Lustre_FOUND)
  if((NOT Lustre_ROOT) AND (NOT (ENV{Lustre_ROOT} STREQUAL "")))
    set(Lustre_ROOT "$ENV{Lustre_ROOT}")
  endif()
  if(Lustre_ROOT)
    set(Lustre_INCLUDE_OPTS HINTS ${Lustre_ROOT}/include NO_DEFAULT_PATHS)
    set(Lustre_LIBRARY_OPTS
      HINTS ${Lustre_ROOT}/lib ${Lustre_ROOT}/lib64
      NO_DEFAULT_PATHS
    )
  endif()

  find_path(Lustre_INCLUDE_DIR lustre/lustreapi.h ${Lustre_INCLUDE_OPTS})
  find_library(Lustre_LIBRARY liblustreapi ${Lustre_LIBRARY_OPTS})

  include(FindPackageHandleStandardArgs)
  find_package_handle_standard_args(Lustre
    FOUND_VAR Lustre_FOUND
    REQUIRED_VARS Lustre_LIBRARY Lustre_INCLUDE_DIR
  )
  if(Lustre_FOUND)
    set(Lustre_INCLUDE_DIRS ${Lustre_INCLUDE_DIR})
    set(Lustre_LIBRARIES ${Lustre_LIBRARY})
    if(Lustre_FOUND AND NOT TARGET lustre::lustre)
      add_library(lustre::lustre UNKNOWN IMPORTED)
      set_target_properties(lustre::lustre PROPERTIES
        IMPORTED_LOCATION             "${Lustre_LIBRARY}"
        INTERFACE_INCLUDE_DIRECTORIES "${Lustre_INCLUDE_DIR}"
      )
    endif()
  endif()
endif()
