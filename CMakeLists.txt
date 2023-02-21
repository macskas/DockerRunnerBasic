cmake_minimum_required(VERSION 2.8)
project(docker-runner)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_FLAGS "-O2 -march=native -std=c++0x")
LINK_LIBRARIES(rt dl)
add_executable(docker-runner main.cpp globals.h cdocker.cpp cdocker.h cdockercontainer.cpp cdockercontainer.h cphprunner.cpp cphprunner.h)
