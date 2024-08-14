cmake_minimum_required(VERSION 3.16)

project(ExamplePayload C CXX)

add_executable(ExamplePayload example.cpp)

set(FLYBY_SDK_PATH "${CMAKE_SOURCE_DIR}")

target_include_directories(ExamplePayload
  PRIVATE 
  "${FLYBY_SDK_PATH}/include"
)

find_library(FLYBY_SDK_LIBRARY
  NAMES FlybySDK
  HINTS ${FLYBY_SDK_PATH}/lib
)

find_package(PkgConfig REQUIRED)
pkg_search_module(gstreamer REQUIRED IMPORTED_TARGET gstreamer-1.0>=1.4)

target_link_libraries(ExamplePayload
  ${FLYBY_SDK_LIBRARY}
  PkgConfig::gstreamer
)

set_target_properties(ExamplePayload PROPERTIES
  RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}"
  INSTALL_RPATH "\$ORIGIN/lib"
  LINK_FLAGS "-Wl,-rpath,\$ORIGIN/lib"
)
