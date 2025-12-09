cmake_minimum_required(VERSION 3.10)
project(integrator_app)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Включаемые директории
include_directories(${CMAKE_SOURCE_DIR}/include)

# Определить OS
if(APPLE)
    set(MACOS TRUE)
elseif(UNIX AND NOT APPLE)
    set(LINUX TRUE)
endif()

# Найти PostgreSQL
find_package(PostgreSQL QUIET)

if(NOT PostgreSQL_FOUND)
    if(MACOS)
        # Для macOS с Homebrew (postgresql@14)
        set(PostgreSQL_INCLUDE_DIR /opt/homebrew/opt/postgresql@14/include)
        set(PostgreSQL_LIBRARY /opt/homebrew/opt/postgresql@14/lib/libpq.dylib)
        include_directories(${PostgreSQL_INCLUDE_DIR})
    elseif(LINUX)
        # Для Linux - ищем стандартные пути
        find_path(PostgreSQL_INCLUDE_DIR libpq-fe.h
            PATHS /usr/include /usr/include/postgresql /usr/local/include
        )
        find_library(PostgreSQL_LIBRARY pq
            PATHS /usr/lib /usr/lib/x86_64-linux-gnu /usr/local/lib
        )
        if(PostgreSQL_INCLUDE_DIR AND PostgreSQL_LIBRARY)
            include_directories(${PostgreSQL_INCLUDE_DIR})
            message(STATUS "PostgreSQL found: ${PostgreSQL_LIBRARY}")
        else()
            message(FATAL_ERROR "PostgreSQL not found. Install with: sudo apt-get install postgresql-client libpq-dev")
        endif()
    endif()
endif()

# Найти OpenSSL
find_package(OpenSSL REQUIRED)

# Источники
set(SOURCES
    src/main.cpp
    src/db.cpp
    src/console.cpp
    src/http_server.cpp
)

# Создать исполняемый файл
add_executable(app ${SOURCES})

# Добавить директории включения для каждой цели
target_include_directories(app PRIVATE ${PostgreSQL_INCLUDE_DIR})
target_include_directories(app PRIVATE ${CMAKE_SOURCE_DIR}/include)

# Линковка библиотек
target_link_libraries(app
    PRIVATE ${PostgreSQL_LIBRARY}
    PRIVATE OpenSSL::Crypto
    PRIVATE pthread
)

# Опции компиляции
if(UNIX)
    target_compile_options(app PRIVATE -Wall -Wextra -O2)
endif()

# Копирование queries.sql в директорию сборки
add_custom_command(TARGET app POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy
    ${CMAKE_SOURCE_DIR}/queries.sql
    ${CMAKE_BINARY_DIR}/queries.sql
    COMMENT "Copying queries.sql to build directory"
)

# Копирование web/ в директорию сборки
add_custom_command(TARGET app POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    ${CMAKE_SOURCE_DIR}/web
    ${CMAKE_BINARY_DIR}/web
    COMMENT "Copying web directory to build directory"
)

message(STATUS "System: ${CMAKE_SYSTEM_NAME}")
message(STATUS "PostgreSQL Library: ${PostgreSQL_LIBRARY}")
message(STATUS "PostgreSQL Include: ${PostgreSQL_INCLUDE_DIR}")
message(STATUS "OpenSSL: ${OPENSSL_CRYPTO_LIBRARY}")
