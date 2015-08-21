cmake_minimum_required(VERSION 3.1 FATAL_ERROR)

project(hashkat-lib CXX)

if (${CMAKE_CXX_COMPILER_ID} STREQUAL MSVC)
  add_definitions(-DWIN32 -D_CRT_SECURE_NO_WARNINGS -D_SCL_SECURE_NO_WARNINGS /EHa /bigobj)
endif()

file(COPY "include" DESTINATION ".")

add_subdirectory(test)
