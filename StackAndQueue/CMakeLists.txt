cmake_minimum_required(VERSION 3.23)
project(StackAndQueue C CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_program(CLANG_TIDY NAMES clang-tidy)
if(CLANG_TIDY)
  set(CMAKE_CXX_CLANG_TIDY "${CLANG_TIDY}")
  message(STATUS "Clang-tidy enabled")
else()
  message(WARNING "Clang-tidy not found, checks disabled")
endif()

set(SOURCES
  ${CMAKE_SOURCE_DIR}/src/main.cpp
  ${CMAKE_SOURCE_DIR}/src/stack_test.cpp
  ${CMAKE_SOURCE_DIR}/src/queue_test.cpp
  ${CMAKE_SOURCE_DIR}/src/benchmark.cpp
)

set(INCLUDE_DIRS
  ${CMAKE_SOURCE_DIR}/src/include
  ${CMAKE_SOURCE_DIR}/src/libs
)

add_executable(${PROJECT_NAME} ${SOURCES})
target_include_directories(${PROJECT_NAME} PRIVATE ${INCLUDE_DIRS})

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++23 -ftemplate-backtrace-limit=0 -O3 ")
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_X_FLAGS_DEBUG} -Wall --pedantic -g3 -ggdb")

find_package(Threads REQUIRED)
target_link_libraries(${PROJECT_NAME} PRIVATE Threads::Threads)

install(TARGETS ${PROJECT_NAME} RUNTIME DESTINATION bin)