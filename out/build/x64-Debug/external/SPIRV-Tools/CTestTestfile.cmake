# CMake generated Testfile for 
# Source directory: C:/Users/THEGM/Documents/Visual Studio 2017/Projects/Omega3D/External/SPIRV-Tools
# Build directory: C:/Users/THEGM/Documents/Visual Studio 2017/Projects/Omega3D/out/build/x64-Debug/External/SPIRV-Tools
# 
# This file includes the relevant testing commands required for 
# testing this directory and lists subdirectories to be tested as well.
if("${CTEST_CONFIGURATION_TYPE}" MATCHES "^([Dd][Ee][Bb][Uu][Gg])$")
  add_test(spirv-tools-copyrights "C:/Users/THEGM/AppData/Local/Programs/Python/Python38-32/python.exe" "utils/check_copyright.py")
  set_tests_properties(spirv-tools-copyrights PROPERTIES  WORKING_DIRECTORY "C:/Users/THEGM/Documents/Visual Studio 2017/Projects/Omega3D/External/SPIRV-Tools" _BACKTRACE_TRIPLES "C:/Users/THEGM/Documents/Visual Studio 2017/Projects/Omega3D/External/SPIRV-Tools/CMakeLists.txt;257;add_test;C:/Users/THEGM/Documents/Visual Studio 2017/Projects/Omega3D/External/SPIRV-Tools/CMakeLists.txt;0;")
else()
  add_test(spirv-tools-copyrights NOT_AVAILABLE)
endif()
subdirs("external")
subdirs("source")
subdirs("tools")
subdirs("test")
subdirs("examples")
