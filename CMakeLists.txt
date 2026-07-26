cmake_minimum_required(VERSION 3.16)

project(BitTorrentClient VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(WIN32)
    message(WARNING
        "BitTorrentClient uses POSIX socket APIs. Build and run on Linux or macOS, or use WSL on Windows.")
endif()

find_package(CURL REQUIRED)
find_package(Threads REQUIRED)

include(FetchContent)

set(CPR_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(CPR_BUILD_TESTS_SSL OFF CACHE BOOL "" FORCE)
set(CPR_USE_SYSTEM_CURL ON CACHE BOOL "" FORCE)

FetchContent_Declare(
    cxxopts
    GIT_REPOSITORY https://github.com/jarro2783/cxxopts.git
    GIT_TAG v3.2.0
)
FetchContent_Declare(
    cpr
    GIT_REPOSITORY https://github.com/libcpr/cpr.git
    GIT_TAG 1.10.5
)
FetchContent_MakeAvailable(cxxopts cpr)

add_library(bencode_lib STATIC
    lib/bencode/BItem.cpp
    lib/bencode/BItemVisitor.cpp
    lib/bencode/BList.cpp
    lib/bencode/BDictionary.cpp
    lib/bencode/BString.cpp
    lib/bencode/BInteger.cpp
    lib/bencode/Decoder.cpp
    lib/bencode/Encoder.cpp
    lib/bencode/PrettyPrinter.cpp
    lib/bencode/Utils.cpp
)
target_include_directories(bencode_lib PUBLIC ${CMAKE_SOURCE_DIR}/lib)

add_library(crypto_lib STATIC
    lib/crypto/sha1.cpp
)
target_include_directories(crypto_lib PUBLIC ${CMAKE_SOURCE_DIR}/lib)

add_library(spdlog_vendor INTERFACE)
target_include_directories(spdlog_vendor INTERFACE ${CMAKE_SOURCE_DIR}/lib)

set(CLIENT_SOURCES
    src/main.cpp
    src/Client.cpp
    src/ConnectToPeer.cpp
    src/RetrivePeers.cpp
    src/TorrentFileParser.cpp
    src/TorrentMessage.cpp
    src/PieceManager.cpp
    src/Piece.cpp
    src/connect.cpp
    src/utils.cpp
)

add_executable(BitTorrentClient ${CLIENT_SOURCES})
target_include_directories(BitTorrentClient PRIVATE ${CMAKE_SOURCE_DIR}/src)
target_link_libraries(BitTorrentClient PRIVATE
    bencode_lib
    crypto_lib
    spdlog_vendor
    cpr::cpr
    cxxopts::cxxopts
    Threads::Threads
)

if(UNIX AND NOT APPLE)
    target_link_libraries(BitTorrentClient PRIVATE rt)
endif()
