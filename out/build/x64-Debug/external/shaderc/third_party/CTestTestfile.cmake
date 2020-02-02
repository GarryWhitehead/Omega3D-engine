# CMake generated Testfile for 
# Source directory: C:/Users/THEGM/Documents/Visual Studio 2017/Projects/Omega3D/External/shaderc/third_party
# Build directory: C:/Users/THEGM/Documents/Visual Studio 2017/Projects/Omega3D/out/build/x64-Debug/External/shaderc/third_party
# 
# This file includes the relevant testing commands required for 
# testing this directory and lists subdirectories to be tested as well.
if("${CTEST_CONFIGURATION_TYPE}" MATCHES "^([Dd][Ee][Bb][Uu][Gg])$")
  add_test(glslang-testsuite "bash" "-o" "igncr" "runtests")
  set_tests_properties(glslang-testsuite PROPERTIES  WORKING_DIRECTORY "C:/Users/THEGM/Documents/Visual Studio 2017/Projects/Omega3D/out/build/x64-Debug/External/shaderc/third_party/Debug/Test" _BACKTRACE_TRIPLES "C:/Users/THEGM/Documents/Visual Studio 2017/Projects/Omega3D/External/shaderc/third_party/CMakeLists.txt;140;add_test;C:/Users/THEGM/Documents/Visual Studio 2017/Projects/Omega3D/External/shaderc/third_party/CMakeLists.txt;0;")
else()
  add_test(glslang-testsuite NOT_AVAILABLE)
endif()
subdirs("googletest")
subdirs("spirv-headers")
subdirs("glslang")
