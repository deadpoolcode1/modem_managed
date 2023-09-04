cmake_minimum_required(VERSION 3.20)
project(modem_host)

set(CMAKE_CXX_STANDARD 20)

set(SOURCES
    Logic.cpp
    CellularManager.cpp
    main.cpp 
)

add_executable(modem_host ${SOURCES})
