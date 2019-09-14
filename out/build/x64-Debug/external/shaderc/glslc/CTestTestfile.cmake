# CMake generated Testfile for 
# Source directory: C:/Users/GARRY/Documents/Visual Studio 2017/Projects/Omega3D/external/shaderc/glslc
# Build directory: C:/Users/GARRY/Documents/Visual Studio 2017/Projects/Omega3D/out/build/x64-Debug/external/shaderc/glslc
# 
# This file includes the relevant testing commands required for 
# testing this directory and lists subdirectories to be tested as well.
if("${CTEST_CONFIGURATION_TYPE}" MATCHES "^([Dd][Ee][Bb][Uu][Gg])$")
  add_test(glslc_file "C:/Users/GARRY/Documents/Visual Studio 2017/Projects/Omega3D/out/build/x64-Debug/external/shaderc/glslc/Debug/glslc_file_test.exe")
else()
  add_test(glslc_file NOT_AVAILABLE)
endif()
if("${CTEST_CONFIGURATION_TYPE}" MATCHES "^([Dd][Ee][Bb][Uu][Gg])$")
  add_test(glslc_resource_parse "C:/Users/GARRY/Documents/Visual Studio 2017/Projects/Omega3D/out/build/x64-Debug/external/shaderc/glslc/Debug/glslc_resource_parse_test.exe")
else()
  add_test(glslc_resource_parse NOT_AVAILABLE)
endif()
if("${CTEST_CONFIGURATION_TYPE}" MATCHES "^([Dd][Ee][Bb][Uu][Gg])$")
  add_test(glslc_stage "C:/Users/GARRY/Documents/Visual Studio 2017/Projects/Omega3D/out/build/x64-Debug/external/shaderc/glslc/Debug/glslc_stage_test.exe")
else()
  add_test(glslc_stage NOT_AVAILABLE)
endif()
subdirs("test")
