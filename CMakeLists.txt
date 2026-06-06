cmake_minimum_required(VERSION 3.15)

project(yottorent)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(CMAKE_BUILD_TYPE Debug)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

include_directories(${CMAKE_SOURCE_DIR})

find_package(Boost REQUIRED CONFIG)

add_executable(yottorent
    main.cpp
    bencode/bencode_parser.cpp
)

target_link_libraries(yottorent PRIVATE Boost::headers)
