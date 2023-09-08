cmake_minimum_required(VERSION 3.26)
project(modem_host)

set(CMAKE_CXX_STANDARD 17)

# Find sdbus-c++
find_package(sdbus-c++ REQUIRED)

set(SOURCES
    Logic.cpp
    CellularManager.cpp
    main.cpp 
)

add_executable(modem_host ${SOURCES})
target_link_libraries(modem_host PRIVATE SDBusCpp::sdbus-c++)