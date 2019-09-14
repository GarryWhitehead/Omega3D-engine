# CMake generated Testfile for 
# Source directory: C:/Users/GARRY/Documents/Visual Studio 2017/Projects/Omega3D/external/shaderc/glslc/test
# Build directory: C:/Users/GARRY/Documents/Visual Studio 2017/Projects/Omega3D/out/build/x64-Debug/external/shaderc/glslc/test
# 
# This file includes the relevant testing commands required for 
# testing this directory and lists subdirectories to be tested as well.
if("${CTEST_CONFIGURATION_TYPE}" MATCHES "^([Dd][Ee][Bb][Uu][Gg])$")
  add_test(glslc_tests "C:/Users/GARRY/AppData/Local/Programs/Python/Python37-32/python.exe" "C:/Users/GARRY/Documents/Visual Studio 2017/Projects/Omega3D/external/shaderc/glslc/test/glslc_test_framework.py" "C:/Users/GARRY/Documents/Visual Studio 2017/Projects/Omega3D/out/build/x64-Debug/external/shaderc/glslc/Debug/glslc.exe" "C:/Users/GARRY/Documents/Visual Studio 2017/Projects/Omega3D/out/build/x64-Debug/external/SPIRV-Tools/tools/Debug/spirv-dis.exe" "--test-dir" "C:/Users/GARRY/Documents/Visual Studio 2017/Projects/Omega3D/external/shaderc/glslc/test")
else()
  add_test(glslc_tests NOT_AVAILABLE)
endif()
