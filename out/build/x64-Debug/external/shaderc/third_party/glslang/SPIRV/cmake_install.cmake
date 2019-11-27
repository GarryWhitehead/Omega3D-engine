# Install script for directory: C:/Users/THEGM/Documents/Visual Studio 2017/Projects/Omega3D/external/glslang/SPIRV

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
  if("${CMAKE_INSTALL_CONFIG_NAME}" MATCHES "^([Dd][Ee][Bb][Uu][Gg])$")
    file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib" TYPE STATIC_LIBRARY FILES "C:/Users/THEGM/Documents/Visual Studio 2017/Projects/Omega3D/out/build/x64-Debug/external/shaderc/third_party/glslang/SPIRV/Debug/SPVRemapperd.lib")
  endif()
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xUnspecifiedx" OR NOT CMAKE_INSTALL_COMPONENT)
  if("${CMAKE_INSTALL_CONFIG_NAME}" MATCHES "^([Dd][Ee][Bb][Uu][Gg])$")
    file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib" TYPE STATIC_LIBRARY FILES "C:/Users/THEGM/Documents/Visual Studio 2017/Projects/Omega3D/out/build/x64-Debug/external/shaderc/third_party/glslang/SPIRV/Debug/SPIRVd.lib")
  endif()
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xUnspecifiedx" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/include/SPIRV" TYPE FILE FILES
    "C:/Users/THEGM/Documents/Visual Studio 2017/Projects/Omega3D/external/glslang/SPIRV/bitutils.h"
    "C:/Users/THEGM/Documents/Visual Studio 2017/Projects/Omega3D/external/glslang/SPIRV/spirv.hpp"
    "C:/Users/THEGM/Documents/Visual Studio 2017/Projects/Omega3D/external/glslang/SPIRV/GLSL.std.450.h"
    "C:/Users/THEGM/Documents/Visual Studio 2017/Projects/Omega3D/external/glslang/SPIRV/GLSL.ext.EXT.h"
    "C:/Users/THEGM/Documents/Visual Studio 2017/Projects/Omega3D/external/glslang/SPIRV/GLSL.ext.KHR.h"
    "C:/Users/THEGM/Documents/Visual Studio 2017/Projects/Omega3D/external/glslang/SPIRV/GlslangToSpv.h"
    "C:/Users/THEGM/Documents/Visual Studio 2017/Projects/Omega3D/external/glslang/SPIRV/hex_float.h"
    "C:/Users/THEGM/Documents/Visual Studio 2017/Projects/Omega3D/external/glslang/SPIRV/Logger.h"
    "C:/Users/THEGM/Documents/Visual Studio 2017/Projects/Omega3D/external/glslang/SPIRV/SpvBuilder.h"
    "C:/Users/THEGM/Documents/Visual Studio 2017/Projects/Omega3D/external/glslang/SPIRV/spvIR.h"
    "C:/Users/THEGM/Documents/Visual Studio 2017/Projects/Omega3D/external/glslang/SPIRV/doc.h"
    "C:/Users/THEGM/Documents/Visual Studio 2017/Projects/Omega3D/external/glslang/SPIRV/SpvTools.h"
    "C:/Users/THEGM/Documents/Visual Studio 2017/Projects/Omega3D/external/glslang/SPIRV/disassemble.h"
    "C:/Users/THEGM/Documents/Visual Studio 2017/Projects/Omega3D/external/glslang/SPIRV/GLSL.ext.AMD.h"
    "C:/Users/THEGM/Documents/Visual Studio 2017/Projects/Omega3D/external/glslang/SPIRV/GLSL.ext.NV.h"
    "C:/Users/THEGM/Documents/Visual Studio 2017/Projects/Omega3D/external/glslang/SPIRV/SPVRemapper.h"
    "C:/Users/THEGM/Documents/Visual Studio 2017/Projects/Omega3D/external/glslang/SPIRV/doc.h"
    )
endif()

