cmake_minimum_required(VERSION 3.20)

project(yottorent)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(CMAKE_BUILD_TYPE Debug)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

include_directories(${CMAKE_SOURCE_DIR})

find_package(Boost REQUIRED COMPONENTS url CONFIG)

include(FetchContent)
FetchContent_Declare(cpr GIT_REPOSITORY https://github.com/libcpr/cpr.git
                         GIT_TAG 1.14.2)
FetchContent_MakeAvailable(cpr)

add_executable(yottorent
    main.cpp
    torrent_file/bencode_parser.cpp
    file_io.cpp
    tracker/tracker.cpp
    peer/handshake/handshake.cpp
    peer/peer_connection.cpp
)
target_link_libraries(yottorent PRIVATE
    Boost::headers
    Boost::url
    cpr::cpr
)
