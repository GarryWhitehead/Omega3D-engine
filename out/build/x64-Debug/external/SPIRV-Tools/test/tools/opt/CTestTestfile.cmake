# CMake generated Testfile for 
# Source directory: C:/Users/GARRY/Documents/Visual Studio 2017/Projects/Omega3D/external/SPIRV-Tools/test/tools/opt
# Build directory: C:/Users/GARRY/Documents/Visual Studio 2017/Projects/Omega3D/out/build/x64-Debug/external/SPIRV-Tools/test/tools/opt
# 
# This file includes the relevant testing commands required for 
# testing this directory and lists subdirectories to be tested as well.
if("${CTEST_CONFIGURATION_TYPE}" MATCHES "^([Dd][Ee][Bb][Uu][Gg])$")
  add_test(spirv_opt_cli_tools_tests "C:/Users/GARRY/AppData/Local/Programs/Python/Python37-32/python.exe" "C:/Users/GARRY/Documents/Visual Studio 2017/Projects/Omega3D/external/SPIRV-Tools/test/tools/opt/../spirv_test_framework.py" "C:/Users/GARRY/Documents/Visual Studio 2017/Projects/Omega3D/out/build/x64-Debug/external/SPIRV-Tools/tools/Debug/spirv-opt.exe" "C:/Users/GARRY/Documents/Visual Studio 2017/Projects/Omega3D/out/build/x64-Debug/external/SPIRV-Tools/tools/Debug/spirv-as.exe" "C:/Users/GARRY/Documents/Visual Studio 2017/Projects/Omega3D/out/build/x64-Debug/external/SPIRV-Tools/tools/Debug/spirv-dis.exe" "--test-dir" "C:/Users/GARRY/Documents/Visual Studio 2017/Projects/Omega3D/external/SPIRV-Tools/test/tools/opt")
else()
  add_test(spirv_opt_cli_tools_tests NOT_AVAILABLE)
endif()
