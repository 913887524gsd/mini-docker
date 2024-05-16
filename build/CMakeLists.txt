cmake_minimum_required(VERSION 3.10)
project(mini-docker)

option(CMAKE_BUILD_TYPE "" debug)

enable_language(CXX)
set(CMAKE_CXX_STANDARD 17)

set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O3")
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -g -O0")

set(PROJECT_DIR "${CMAKE_SOURCE_DIR}/..")

include_directories("${PROJECT_DIR}/inc")
add_compile_options(-Wall -Wshadow -Werror -Wno-format)

file(GLOB_RECURSE SRC_FILES
    "${PROJECT_DIR}/src/*.cpp"
)

set(DYN_LIB)

set(EXE_NAME "mini-docker")
add_executable(${EXE_NAME} ${SRC_FILES})
set_target_properties(${EXE_NAME} PROPERTIES LINKER_LANGUAGE CXX)
target_link_libraries(${EXE_NAME} ${DYN_LIB})