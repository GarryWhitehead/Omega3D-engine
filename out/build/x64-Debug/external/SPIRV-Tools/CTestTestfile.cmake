# CMake generated Testfile for 
# Source directory: C:/Users/GARRY/Documents/Visual Studio 2017/Projects/Omega3D/external/SPIRV-Tools
# Build directory: C:/Users/GARRY/Documents/Visual Studio 2017/Projects/Omega3D/out/build/x64-Debug/external/SPIRV-Tools
# 
# This file includes the relevant testing commands required for 
# testing this directory and lists subdirectories to be tested as well.
if("${CTEST_CONFIGURATION_TYPE}" MATCHES "^([Dd][Ee][Bb][Uu][Gg])$")
  add_test(spirv-tools-copyrights "C:/Users/GARRY/AppData/Local/Programs/Python/Python37-32/python.exe" "utils/check_copyright.py")
  set_tests_properties(spirv-tools-copyrights PROPERTIES  WORKING_DIRECTORY "C:/Users/GARRY/Documents/Visual Studio 2017/Projects/Omega3D/external/SPIRV-Tools")
else()
  add_test(spirv-tools-copyrights NOT_AVAILABLE)
endif()
subdirs("external")
subdirs("source")
subdirs("tools")
subdirs("test")
subdirs("examples")
