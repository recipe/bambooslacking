cmake_minimum_required(VERSION 3.15)
project(bambooslacking)
set(CMAKE_CXX_STANDARD 17)

set(cpprestsdk_DIR /usr/lib/${CMAKE_LIBRARY_ARCHITECTURE}/cmake/)

find_path(
        LevelDB_INCLUDE NAMES leveldb/db.h
        PATHS $ENV{LEVELDB_ROOT}/include /opt/local/include /usr/local/include /usr/include
        DOC "Path in which the file leveldb/db.h is located."
)
find_library(LevelDB_LIBRARY NAMES leveldb
        PATHS /usr/lib $ENV{LEVELDB_ROOT}/lib
        DOC "Path to leveldb library."
)
include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(LevelDB DEFAULT_MSG LevelDB_INCLUDE LevelDB_LIBRARY)

if(LEVELDB_FOUND)
    message(STATUS "Found LevelDB (include: ${LevelDB_INCLUDE}, library: ${LevelDB_LIBRARY})")
endif()

find_package(cpprestsdk REQUIRED)
find_package(Boost REQUIRED COMPONENTS atomic chrono thread exception)

set(TARGET_SOURCES
        base64.cc easylogging++.cc slackapi.cc bamboohrapi.cc encryption.cc db.cc
        app.cc common.cc network_utils.cc basic_controller.cc app_controller.cc uri.cc
)
list(TRANSFORM TARGET_SOURCES PREPEND "./src/")

add_executable(bambooslacking ./src/main.cc ${TARGET_SOURCES})

set(BAMBOOSLACKING_INCLUDE_DIR "./src/include")
target_include_directories(bambooslacking PRIVATE ${BAMBOOSLACKING_INCLUDE_DIR})
target_link_libraries(
        bambooslacking PRIVATE
        cpprestsdk::cpprest leveldb
        Boost::atomic Boost::chrono Boost::exception Boost::thread Boost::date_time
)

# it's added for the tests only
add_executable(test ./src/testmain.cc ./src/common.cc)
target_include_directories(test PRIVATE ${BAMBOOSLACKING_INCLUDE_DIR})
target_link_libraries(
        test PRIVATE
        cpprestsdk::cpprest leveldb
        Boost::atomic Boost::chrono Boost::exception Boost::thread Boost::date_time
)



