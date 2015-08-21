cmake_minimum_required(VERSION 3.1 FATAL_ERROR)

project(hashkat-lib CXX)

file(GLOB_RECURSE HASHKAT_LIB_HEADERS 
	include/*.hxx
)
add_custom_target(libhashkat SOURCES ${HASHKAT_LIB_HEADERS})

#add_library(LIBHASHKAT STATIC include/network.h include/action.h)
#set_target_properties(LIBHASHKAT PROPERTIES LINKER_LANGUAGE CXX)

if (${CMAKE_CXX_COMPILER_ID} STREQUAL MSVC)
  add_definitions(-DWIN32 -D_CRT_SECURE_NO_WARNINGS -D_SCL_SECURE_NO_WARNINGS /EHa /bigobj)
endif()

#file(COPY "include" DESTINATION ".")

add_subdirectory(test)
