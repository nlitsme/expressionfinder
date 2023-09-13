cmake_minimum_required(VERSION 3.23)
project(findexpr)
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake_find")

find_package(cpputils REQUIRED)


add_executable(findexpr findexpr.cpp)
target_link_libraries(findexpr cpputils)

