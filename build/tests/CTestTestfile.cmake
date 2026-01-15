# CMake generated Testfile for 
# Source directory: C:/McMaker Projects/Tools/libs/std_execution/tests
# Build directory: C:/McMaker Projects/Tools/libs/std_execution/build/tests
# 
# This file includes the relevant testing commands required for 
# testing this directory and lists subdirectories to be tested as well.
if(CTEST_CONFIGURATION_TYPE MATCHES "^([Dd][Ee][Bb][Uu][Gg])$")
  add_test([=[DualStackNet26Tests]=] "C:/McMaker Projects/Tools/libs/std_execution/build/tests/Debug/dualstack_tests.exe")
  set_tests_properties([=[DualStackNet26Tests]=] PROPERTIES  _BACKTRACE_TRIPLES "C:/McMaker Projects/Tools/libs/std_execution/tests/CMakeLists.txt;15;add_test;C:/McMaker Projects/Tools/libs/std_execution/tests/CMakeLists.txt;0;")
elseif(CTEST_CONFIGURATION_TYPE MATCHES "^([Rr][Ee][Ll][Ee][Aa][Ss][Ee])$")
  add_test([=[DualStackNet26Tests]=] "C:/McMaker Projects/Tools/libs/std_execution/build/tests/Release/dualstack_tests.exe")
  set_tests_properties([=[DualStackNet26Tests]=] PROPERTIES  _BACKTRACE_TRIPLES "C:/McMaker Projects/Tools/libs/std_execution/tests/CMakeLists.txt;15;add_test;C:/McMaker Projects/Tools/libs/std_execution/tests/CMakeLists.txt;0;")
elseif(CTEST_CONFIGURATION_TYPE MATCHES "^([Mm][Ii][Nn][Ss][Ii][Zz][Ee][Rr][Ee][Ll])$")
  add_test([=[DualStackNet26Tests]=] "C:/McMaker Projects/Tools/libs/std_execution/build/tests/MinSizeRel/dualstack_tests.exe")
  set_tests_properties([=[DualStackNet26Tests]=] PROPERTIES  _BACKTRACE_TRIPLES "C:/McMaker Projects/Tools/libs/std_execution/tests/CMakeLists.txt;15;add_test;C:/McMaker Projects/Tools/libs/std_execution/tests/CMakeLists.txt;0;")
elseif(CTEST_CONFIGURATION_TYPE MATCHES "^([Rr][Ee][Ll][Ww][Ii][Tt][Hh][Dd][Ee][Bb][Ii][Nn][Ff][Oo])$")
  add_test([=[DualStackNet26Tests]=] "C:/McMaker Projects/Tools/libs/std_execution/build/tests/RelWithDebInfo/dualstack_tests.exe")
  set_tests_properties([=[DualStackNet26Tests]=] PROPERTIES  _BACKTRACE_TRIPLES "C:/McMaker Projects/Tools/libs/std_execution/tests/CMakeLists.txt;15;add_test;C:/McMaker Projects/Tools/libs/std_execution/tests/CMakeLists.txt;0;")
else()
  add_test([=[DualStackNet26Tests]=] NOT_AVAILABLE)
endif()
