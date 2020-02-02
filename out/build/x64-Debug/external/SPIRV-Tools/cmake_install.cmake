# Install script for directory: C:/Users/THEGM/Documents/Visual Studio 2017/Projects/Omega3D/External/SPIRV-Tools

# Set the install prefix
if(NOT DEFINED CMAKE_INSTALL_PREFIX)
  set(CMAKE_INSTALL_PREFIX "C:/Users/THEGM/Documents/Visual Studio 2017/Projects/Omega3D/out/install/x64-Debug")
endif()
string(REGEX REPLACE "/$" "" CMAKE_INSTALL_PREFIX "${CMAKE_INSTALL_PREFIX}")

# Set the install configuration name.
if(NOT DEFINED CMAKE_INSTALL_CONFIG_NAME)
  if(BUILD_TYPE)
    string(REGEX REPLACE "^[^A-Za-z0-9_]+" ""
           CMAKE_INSTALL_CONFIG_NAME "${BUILD_TYPE}")
  else()
    set(CMAKE_INSTALL_CONFIG_NAME "Debug")
  endif()
  message(STATUS "Install configuration: \"${CMAKE_INSTALL_CONFIG_NAME}\"")
endif()

# Set the component getting installed.
if(NOT CMAKE_INSTALL_COMPONENT)
  if(COMPONENT)
    message(STATUS "Install component: \"${COMPONENT}\"")
    set(CMAKE_INSTALL_COMPONENT "${COMPONENT}")
  else()
    set(CMAKE_INSTALL_COMPONENT)
  endif()
endif()

# Is this installation the result of a crosscompile?
if(NOT DEFINED CMAKE_CROSSCOMPILING)
  set(CMAKE_CROSSCOMPILING "FALSE")
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xUnspecifiedx" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/include/spirv-tools" TYPE FILE FILES
    "C:/Users/THEGM/Documents/Visual Studio 2017/Projects/Omega3D/External/SPIRV-Tools/include/spirv-tools/libspirv.h"
    "C:/Users/THEGM/Documents/Visual Studio 2017/Projects/Omega3D/External/SPIRV-Tools/include/spirv-tools/libspirv.hpp"
    "C:/Users/THEGM/Documents/Visual Studio 2017/Projects/Omega3D/External/SPIRV-Tools/include/spirv-tools/optimizer.hpp"
    "C:/Users/THEGM/Documents/Visual Studio 2017/Projects/Omega3D/External/SPIRV-Tools/include/spirv-tools/linker.hpp"
    "C:/Users/THEGM/Documents/Visual Studio 2017/Projects/Omega3D/External/SPIRV-Tools/include/spirv-tools/instrument.hpp"
    )
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xUnspecifiedx" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib/pkgconfig" TYPE FILE FILES
    "C:/Users/THEGM/Documents/Visual Studio 2017/Projects/Omega3D/out/build/x64-Debug/External/SPIRV-Tools/SPIRV-Tools.pc"
    "C:/Users/THEGM/Documents/Visual Studio 2017/Projects/Omega3D/out/build/x64-Debug/External/SPIRV-Tools/SPIRV-Tools-shared.pc"
    )
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for each subdirectory.
  include("C:/Users/THEGM/Documents/Visual Studio 2017/Projects/Omega3D/out/build/x64-Debug/External/SPIRV-Tools/external/cmake_install.cmake")
  include("C:/Users/THEGM/Documents/Visual Studio 2017/Projects/Omega3D/out/build/x64-Debug/External/SPIRV-Tools/source/cmake_install.cmake")
  include("C:/Users/THEGM/Documents/Visual Studio 2017/Projects/Omega3D/out/build/x64-Debug/External/SPIRV-Tools/tools/cmake_install.cmake")
  include("C:/Users/THEGM/Documents/Visual Studio 2017/Projects/Omega3D/out/build/x64-Debug/External/SPIRV-Tools/test/cmake_install.cmake")
  include("C:/Users/THEGM/Documents/Visual Studio 2017/Projects/Omega3D/out/build/x64-Debug/External/SPIRV-Tools/examples/cmake_install.cmake")

endif()

