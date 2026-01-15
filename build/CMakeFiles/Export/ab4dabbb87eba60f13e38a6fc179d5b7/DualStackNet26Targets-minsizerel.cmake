#----------------------------------------------------------------
# Generated CMake target import file for configuration "MinSizeRel".
#----------------------------------------------------------------

# Commands may need to know the format version.
set(CMAKE_IMPORT_FILE_VERSION 1)

# Import target "DualStackNet26::dualstack_net26" for configuration "MinSizeRel"
set_property(TARGET DualStackNet26::dualstack_net26 APPEND PROPERTY IMPORTED_CONFIGURATIONS MINSIZEREL)
set_target_properties(DualStackNet26::dualstack_net26 PROPERTIES
  IMPORTED_IMPLIB_MINSIZEREL "${_IMPORT_PREFIX}/lib/dualstack_net26.lib"
  IMPORTED_LOCATION_MINSIZEREL "${_IMPORT_PREFIX}/bin/dualstack_net26.dll"
  )

list(APPEND _cmake_import_check_targets DualStackNet26::dualstack_net26 )
list(APPEND _cmake_import_check_files_for_DualStackNet26::dualstack_net26 "${_IMPORT_PREFIX}/lib/dualstack_net26.lib" "${_IMPORT_PREFIX}/bin/dualstack_net26.dll" )

# Commands beyond this point should not need to know the version.
set(CMAKE_IMPORT_FILE_VERSION)
