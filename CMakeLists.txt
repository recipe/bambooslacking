cmake_minimum_required(VERSION 3.15)
project(bambooslacking)
set(CMAKE_CXX_STANDARD 17)

option(test "Build all tests" OFF)

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

### unit testing
if (test)
    find_path(
        gtest_INCLUDE NAMES gtest/gtest.h
        PATHS /usr/include
        DOC "Path in which the file /usr/include/gtest/gtest.h is located."
    )

    find_library(gtest_LIBRARY NAMES gtest
        PATHS /usr/lib
        DOC "Path to gtest library."
    )

    include(FindPackageHandleStandardArgs)
    find_package_handle_standard_args(gtest DEFAULT_MSG gtest_INCLUDE gtest_LIBRARY)

    if(gtest_FOUND)
        message(STATUS "Found gtest (include: ${gtest_INCLUDE}, library: ${gtest_LIBRARY})")
    endif()

    include_directories(${gtest_SOURCE_DIR}/include ${gtest_SOURCE_DIR})

    enable_testing()

    add_executable(bambooslacking-test ./test/main.cc)

    target_include_directories(bambooslacking-test PRIVATE ${BAMBOOSLACKING_INCLUDE_DIR})

    target_link_libraries(
        bambooslacking-test PRIVATE
        cpprestsdk::cpprest leveldb
        Boost::atomic Boost::chrono Boost::exception Boost::thread Boost::date_time
        gtest gtest_main
    )

    add_test(NAME bambooslacingTest COMMAND bambooslacking-test -v)
endif()
