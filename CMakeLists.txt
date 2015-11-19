cmake_minimum_required(VERSION 2.6)
project(pkgfs)

add_executable(pkgfs main.cpp)

install(TARGETS pkgfs RUNTIME DESTINATION bin)

add_subdirectory(utils)
