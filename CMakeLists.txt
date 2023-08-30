cmake_minimum_required(VERSION 3.26)
project(modem_host)

set(CMAKE_CXX_STANDARD 20)

# Find sdbus-c++
find_package(sdbus-c++ REQUIRED)

# Find dbus-c++
find_package(PkgConfig REQUIRED)
pkg_check_modules(DBUS REQUIRED dbus-1 dbus-c++-1)
include_directories(${DBUS_INCLUDE_DIRS})
link_directories(${DBUS_LIBRARY_DIRS})

set(SOURCES
    Logic.cpp
    CellularManager.cpp
    main.cpp 
)

add_executable(modem_host ${SOURCES})
target_link_libraries(modem_host PRIVATE SDBusCpp::sdbus-c++ ${DBUS_LIBRARIES})