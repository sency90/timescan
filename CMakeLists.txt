cmake_minimum_required(VERSION 3.10)
project(timescan_test)

set(CMAKE_CXX_STANDARD 17)

# gtest & gmock
find_package(GTest REQUIRED)

include_directories(
  /usr/include
  /usr/local/include
  src
)

link_directories(
  /usr/lib
)

add_executable(
  timescan_test
  test/timescan_test.cpp
)

target_link_libraries(timescan_test
  gtest
  gmock
  pthread
)

