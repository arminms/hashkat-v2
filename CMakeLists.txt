cmake_minimum_required(VERSION 3.1 FATAL_ERROR)

project(hashkat-lib LANGUAGES CXX)

if (CMAKE_CXX_COMPILER_ID MATCHES Clang)
    add_definitions(-stdlib=libc++ -std=c++11)
elseif (${CMAKE_CXX_COMPILER_ID} STREQUAL GNU)
    add_definitions(-std=c++11)
elseif (${CMAKE_CXX_COMPILER_ID} STREQUAL Intel)
    add_definitions(-std=c++11)
elseif (${CMAKE_CXX_COMPILER_ID} STREQUAL MSVC)
  add_definitions(-DWIN32 -D_CRT_SECURE_NO_WARNINGS -D_SCL_SECURE_NO_WARNINGS /EHa)
endif()

file(COPY "include" DESTINATION ".")

add_subdirectory(test)