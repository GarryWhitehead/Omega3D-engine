# CMake generated Testfile for 
# Source directory: C:/Users/THEGM/Documents/Visual Studio 2017/Projects/Omega3D/external/glslang/gtests
# Build directory: C:/Users/THEGM/Documents/Visual Studio 2017/Projects/Omega3D/out/build/x64-Debug/external/shaderc/third_party/glslang/gtests
# 
# This file includes the relevant testing commands required for 
# testing this directory and lists subdirectories to be tested as well.
if("${CTEST_CONFIGURATION_TYPE}" MATCHES "^([Dd][Ee][Bb][Uu][Gg])$")
  add_test(glslang-gtests "C:/Users/THEGM/Documents/Visual Studio 2017/Projects/Omega3D/out/build/x64-Debug/external/shaderc/third_party/glslang/gtests/Debug/glslangtests.exe" "--test-root" "C:/Users/THEGM/Documents/Visual Studio 2017/Projects/Omega3D/external/glslang/gtests/../Test")
  set_tests_properties(glslang-gtests PROPERTIES  _BACKTRACE_TRIPLES "C:/Users/THEGM/Documents/Visual Studio 2017/Projects/Omega3D/external/glslang/gtests/CMakeLists.txt;58;add_test;C:/Users/THEGM/Documents/Visual Studio 2017/Projects/Omega3D/external/glslang/gtests/CMakeLists.txt;0;")
else()
  add_test(glslang-gtests NOT_AVAILABLE)
endif()
