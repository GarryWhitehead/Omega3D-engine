# CMake generated Testfile for 
# Source directory: C:/Users/GARRY/Documents/Visual Studio 2017/Projects/Omega3D/external/shaderc/third_party
# Build directory: C:/Users/GARRY/Documents/Visual Studio 2017/Projects/Omega3D/out/build/x64-Debug/external/shaderc/third_party
# 
# This file includes the relevant testing commands required for 
# testing this directory and lists subdirectories to be tested as well.
if("${CTEST_CONFIGURATION_TYPE}" MATCHES "^([Dd][Ee][Bb][Uu][Gg])$")
  add_test(glslang-testsuite "bash" "-o" "igncr" "runtests")
  set_tests_properties(glslang-testsuite PROPERTIES  WORKING_DIRECTORY "C:/Users/GARRY/Documents/Visual Studio 2017/Projects/Omega3D/out/build/x64-Debug/external/shaderc/third_party/Debug/Test")
else()
  add_test(glslang-testsuite NOT_AVAILABLE)
endif()
subdirs("googletest")
subdirs("spirv-headers")
subdirs("glslang")
