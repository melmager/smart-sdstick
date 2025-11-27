cmake_minimum_required(VERSION 4.0)
project(s32_otg)

set(CMAKE_CXX_STANDARD 20)

add_executable(s32_otg main.cpp
        factory/main/info/info_type.h
        factory/main/info/paragraph_renderer.c)
