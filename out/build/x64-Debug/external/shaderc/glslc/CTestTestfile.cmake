# CMake generated Testfile for 
# Source directory: C:/Users/THEGM/Documents/Visual Studio 2017/Projects/Omega3D/external/shaderc/glslc
# Build directory: C:/Users/THEGM/Documents/Visual Studio 2017/Projects/Omega3D/out/build/x64-Debug/external/shaderc/glslc
# 
# This file includes the relevant testing commands required for 
# testing this directory and lists subdirectories to be tested as well.
if("${CTEST_CONFIGURATION_TYPE}" MATCHES "^([Dd][Ee][Bb][Uu][Gg])$")
  add_test(glslc_file "C:/Users/THEGM/Documents/Visual Studio 2017/Projects/Omega3D/out/build/x64-Debug/external/shaderc/glslc/Debug/glslc_file_test.exe")
  set_tests_properties(glslc_file PROPERTIES  _BACKTRACE_TRIPLES "C:/Users/THEGM/Documents/Visual Studio 2017/Projects/Omega3D/external/shaderc/cmake/utils.cmake;126;add_test;C:/Users/THEGM/Documents/Visual Studio 2017/Projects/Omega3D/external/shaderc/glslc/CMakeLists.txt;31;shaderc_add_tests;C:/Users/THEGM/Documents/Visual Studio 2017/Projects/Omega3D/external/shaderc/glslc/CMakeLists.txt;0;")
else()
  add_test(glslc_file NOT_AVAILABLE)
endif()
if("${CTEST_CONFIGURATION_TYPE}" MATCHES "^([Dd][Ee][Bb][Uu][Gg])$")
  add_test(glslc_resource_parse "C:/Users/THEGM/Documents/Visual Studio 2017/Projects/Omega3D/out/build/x64-Debug/external/shaderc/glslc/Debug/glslc_resource_parse_test.exe")
  set_tests_properties(glslc_resource_parse PROPERTIES  _BACKTRACE_TRIPLES "C:/Users/THEGM/Documents/Visual Studio 2017/Projects/Omega3D/external/shaderc/cmake/utils.cmake;126;add_test;C:/Users/THEGM/Documents/Visual Studio 2017/Projects/Omega3D/external/shaderc/glslc/CMakeLists.txt;31;shaderc_add_tests;C:/Users/THEGM/Documents/Visual Studio 2017/Projects/Omega3D/external/shaderc/glslc/CMakeLists.txt;0;")
else()
  add_test(glslc_resource_parse NOT_AVAILABLE)
endif()
if("${CTEST_CONFIGURATION_TYPE}" MATCHES "^([Dd][Ee][Bb][Uu][Gg])$")
  add_test(glslc_stage "C:/Users/THEGM/Documents/Visual Studio 2017/Projects/Omega3D/out/build/x64-Debug/external/shaderc/glslc/Debug/glslc_stage_test.exe")
  set_tests_properties(glslc_stage PROPERTIES  _BACKTRACE_TRIPLES "C:/Users/THEGM/Documents/Visual Studio 2017/Projects/Omega3D/external/shaderc/cmake/utils.cmake;126;add_test;C:/Users/THEGM/Documents/Visual Studio 2017/Projects/Omega3D/external/shaderc/glslc/CMakeLists.txt;31;shaderc_add_tests;C:/Users/THEGM/Documents/Visual Studio 2017/Projects/Omega3D/external/shaderc/glslc/CMakeLists.txt;0;")
else()
  add_test(glslc_stage NOT_AVAILABLE)
endif()
subdirs("test")
