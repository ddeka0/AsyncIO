cmake_minimum_required (VERSION 3.5)
set(CMAKE_BUILD_TYPE Debug)

project(liburing-test)
message(STATUS "Building... : ${PROJECT_NAME}")

SET(CMAKE_CXX_COMPILER g++-10)

message(STATUS "Project Directory: ${PROJECT_SOURCE_DIR}")

set(CMAKE_BINARY_DIR ${CMAKE_SOURCE_DIR}/build)
set(EXECUTABLE_OUTPUT_PATH ${CMAKE_BINARY_DIR}/bin)

include_directories(./include/)

add_executable(${PROJECT_NAME}
    ./test/timerApp.cpp
    ./src/asyncServer.cpp
)


set(BASIC_LIBRARIES "-luring -pthread")
set(CMAKE_CXX_FLAGS "-std=c++20 -Wall -ggdb -O2 -fsanitize=address -fno-omit-frame-pointer")



target_link_libraries(${PROJECT_NAME}  
                    ${liburing_location}
                    ${BASIC_LIBRARIES}
)