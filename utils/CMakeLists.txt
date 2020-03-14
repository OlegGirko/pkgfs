find_package(Boost 1.60 REQUIRED)

add_executable(rpminspect rpminspect.cpp)
set_property(TARGET rpminspect PROPERTY CXX_STANDARD 11)
target_include_directories(rpminspect PUBLIC ${Boost_INCLUDE_DIRS})
