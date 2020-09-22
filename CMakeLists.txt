cmake_minimum_required(VERSION 3.16)
project(DamperNode)
set(CMAKE_CXX_STANDARD 14)

# Maya Dev Kit - Include the project setting file / include path
include($ENV{DEVKIT_LOCATION}/cmake/pluginEntry.cmake)

set(SOURCE_FILES
        DamperNode.h
        DamperNode.cpp)

# Maya Dev Kit - add the plugin building function name
build_plugin()