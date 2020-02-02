# CMake generated Testfile for 
# Source directory: C:/Users/THEGM/Documents/Visual Studio 2017/Projects/Omega3D/External/SPIRV-Tools/test/tools/opt
# Build directory: C:/Users/THEGM/Documents/Visual Studio 2017/Projects/Omega3D/out/build/x64-Debug/External/SPIRV-Tools/test/tools/opt
# 
# This file includes the relevant testing commands required for 
# testing this directory and lists subdirectories to be tested as well.
if("${CTEST_CONFIGURATION_TYPE}" MATCHES "^([Dd][Ee][Bb][Uu][Gg])$")
  add_test(spirv_opt_cli_tools_tests "C:/Users/THEGM/AppData/Local/Programs/Python/Python38-32/python.exe" "C:/Users/THEGM/Documents/Visual Studio 2017/Projects/Omega3D/External/SPIRV-Tools/test/tools/opt/../spirv_test_framework.py" "C:/Users/THEGM/Documents/Visual Studio 2017/Projects/Omega3D/out/build/x64-Debug/External/SPIRV-Tools/tools/Debug/spirv-opt.exe" "C:/Users/THEGM/Documents/Visual Studio 2017/Projects/Omega3D/out/build/x64-Debug/External/SPIRV-Tools/tools/Debug/spirv-as.exe" "C:/Users/THEGM/Documents/Visual Studio 2017/Projects/Omega3D/out/build/x64-Debug/External/SPIRV-Tools/tools/Debug/spirv-dis.exe" "--test-dir" "C:/Users/THEGM/Documents/Visual Studio 2017/Projects/Omega3D/External/SPIRV-Tools/test/tools/opt")
  set_tests_properties(spirv_opt_cli_tools_tests PROPERTIES  _BACKTRACE_TRIPLES "C:/Users/THEGM/Documents/Visual Studio 2017/Projects/Omega3D/External/SPIRV-Tools/test/tools/opt/CMakeLists.txt;17;add_test;C:/Users/THEGM/Documents/Visual Studio 2017/Projects/Omega3D/External/SPIRV-Tools/test/tools/opt/CMakeLists.txt;0;")
else()
  add_test(spirv_opt_cli_tools_tests NOT_AVAILABLE)
endif()
