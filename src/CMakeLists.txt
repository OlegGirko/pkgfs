add_executable(pkgfs main.cpp)
install(TARGETS pkgfs RUNTIME DESTINATION bin)
set_property(TARGET pkgfs PROPERTY CXX_STANDARD 11)
