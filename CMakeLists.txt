cmake_minimum_required(VERSION 3.14)
project(pkgfs)

add_executable(pkgfs main.cpp)

install(TARGETS pkgfs RUNTIME DESTINATION bin)

add_subdirectory(utils)
