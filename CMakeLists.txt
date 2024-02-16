cmake_minimum_required(VERSION 3.27)
project(Horizon)

set(CMAKE_CXX_STANDARD 17)

add_executable(Horizon WIN32 main.cpp resource.rc
        keyHook.cpp
        keyHook.h
        tray.cpp
        tray.h)
