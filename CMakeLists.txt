cmake_minimum_required(VERSION 3.14)
project(proxy C)

set(CMAKE_BUILD_TYPE Debug)
set(CMAKE_C_FLAGS_DEBUG "-g")
set(CMAKE_C_STANDARD 99)

include(FetchContent)
include(ExternalProject)

FetchContent_Declare(
    prometheus
    GIT_REPOSITORY https://github.com/polina-tereschenko/prometheus-client-c.git
    GIT_TAG        master
)
FetchContent_MakeAvailable(prometheus)
if(NOT prometheus_POPULATED)
    FetchContent_Populate(prometheus)
endif()

FetchContent_Declare(
    argtable3
    GIT_REPOSITORY https://github.com/argtable/argtable3.git
    GIT_TAG v3.3.1
)
FetchContent_MakeAvailable(argtable3)
if(NOT argtable3_POPULATED)
    FetchContent_Populate(argtable3)
endif()

add_subdirectory(${prometheus_SOURCE_DIR}/prom)
add_executable(proxy proxy.c)

target_include_directories(proxy PRIVATE
    ${prometheus_SOURCE_DIR}/prom/include
)

target_link_libraries(proxy PRIVATE
    argtable3
    prom
)
