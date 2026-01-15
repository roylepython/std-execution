
####### Expanded from @PACKAGE_INIT@ by configure_package_config_file() #######
####### Any changes to this file will be overwritten by the next CMake run ####
####### The input file was DualStackNet26Config.cmake.in                            ########

get_filename_component(PACKAGE_PREFIX_DIR "${CMAKE_CURRENT_LIST_DIR}/../../../" ABSOLUTE)

macro(set_and_check _var _file)
  set(${_var} "${_file}")
  if(NOT EXISTS "${_file}")
    message(FATAL_ERROR "File or directory ${_file} referenced by variable ${_var} does not exist !")
  endif()
endmacro()

macro(check_required_components _NAME)
  foreach(comp ${${_NAME}_FIND_COMPONENTS})
    if(NOT ${_NAME}_${comp}_FOUND)
      if(${_NAME}_FIND_REQUIRED_${comp})
        set(${_NAME}_FOUND FALSE)
      endif()
    endif()
  endforeach()
endmacro()

####################################################################################

include("${CMAKE_CURRENT_LIST_DIR}/DualStackNet26Targets.cmake")

check_required_components(DualStackNet26)

# Set include directories
set(DualStackNet26_INCLUDE_DIRS "${PACKAGE_PREFIX_DIR}/include")

# Set library directories
set(DualStackNet26_LIBRARIES dualstack_net26)

# Set compile definitions
set(DualStackNet26_DEFINITIONS "")

# Set version information
set(DualStackNet26_VERSION_MAJOR 1)
set(DualStackNet26_VERSION_MINOR 0)
set(DualStackNet26_VERSION_PATCH 0)
set(DualStackNet26_VERSION "1.0.0")

# Check for required features
include(CMakeFindDependencyMacro)

# Find dependencies based on platform
if(1)
    # Windows dependencies
    set(DualStackNet26_PLATFORM_LIBS ws2_32)
else()
    # Unix dependencies
    set(DualStackNet26_PLATFORM_LIBS pthread)
endif()

# Check for C++26 features
if(NOT CMAKE_CXX_STANDARD EQUAL 26)
    message(WARNING "DualStackNet26 recommends C++26 for full feature support")
endif()
