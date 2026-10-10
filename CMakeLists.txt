cmake_minimum_required(VERSION 3.20)

project(MathanMart CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(WIN32)
    list(APPEND CMAKE_PREFIX_PATH
        "${CMAKE_CURRENT_SOURCE_DIR}/drogon/install-pg"
        "C:/msys64/ucrt64"
    )
endif()

add_subdirectory(backend)
