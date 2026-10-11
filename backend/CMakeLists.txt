cmake_minimum_required(VERSION 3.20)

project(MathanMartBackend CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(WIN32)
    # Use the local PG-enabled Drogon build for Windows MinGW
    list(APPEND CMAKE_PREFIX_PATH
        "${CMAKE_CURRENT_SOURCE_DIR}/../drogon/install-pg"
        "C:/msys64/ucrt64"
    )
    link_directories(
        "C:/msys64/ucrt64/lib"
        "C:/Program Files/PostgreSQL/18/lib"
    )
endif()

find_package(Drogon CONFIG REQUIRED)

set(BACKEND_SOURCES
    main.cpp
    services/Database.cpp
    services/AuthService.cpp
    services/ProductService.cpp
    services/OrderService.cpp
    middleware/AuthMiddleware.cpp
    controllers/AuthController.cpp
    controllers/ProductController.cpp
    controllers/CartController.cpp
    controllers/OrderController.cpp
    controllers/ReviewController.cpp
    controllers/AdminController.cpp
)

add_executable(MathanMartBackend ${BACKEND_SOURCES})

if(WIN32)
    target_include_directories(MathanMartBackend PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/models
        ${CMAKE_CURRENT_SOURCE_DIR}/services
        ${CMAKE_CURRENT_SOURCE_DIR}/controllers
        ${CMAKE_CURRENT_SOURCE_DIR}/middleware
        "C:/msys64/ucrt64/include"
        "C:/Program Files/PostgreSQL/18/include"
    )
    target_link_libraries(MathanMartBackend PRIVATE
        Drogon::Drogon
        pq          # PostgreSQL client library (used by Mathan Mart application)
        sqlite3     # Satisfies Drogon's static library internal symbols
        crypto
        ssl
    )
else()
    find_package(PostgreSQL REQUIRED)
    find_package(OpenSSL REQUIRED)
    find_package(Threads REQUIRED)
    target_include_directories(MathanMartBackend PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/models
        ${CMAKE_CURRENT_SOURCE_DIR}/services
        ${CMAKE_CURRENT_SOURCE_DIR}/controllers
        ${CMAKE_CURRENT_SOURCE_DIR}/middleware
        ${PostgreSQL_INCLUDE_DIRS}
    )
    target_link_libraries(MathanMartBackend PRIVATE
        Drogon::Drogon
        ${PostgreSQL_LIBRARIES}
        OpenSSL::Crypto
        OpenSSL::SSL
        Threads::Threads
    )
endif()
