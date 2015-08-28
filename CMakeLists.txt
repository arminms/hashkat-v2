CMAKE_MINIMUM_REQUIRED(VERSION 2.4 FATAL_ERROR)

PROJECT(hashkat-lib CXX)

IF(WIN32)
    SET(USERLIB_PREFIX_DEFAULT "c:/local")
ELSE(WIN32)
    SET(USERLIB_PREFIX_DEFAULT "/usr")
ENDIF(WIN32)

#IF(WIN32)
#    SET(BOOST_PREFIX_DEFAULT "c:/Program Files/Boost")
#    OPTION(BOOST_DYNAMIC "Link to boost DLLs (OFF means static link)" OFF)
#ELSE(WIN32)
#    SET(BOOST_PREFIX_DEFAULT ${USERLIB_PREFIX})
#ENDIF(WIN32)

#INCLUDE(cmake/HkFindBoost.txt)
INCLUDE(cmake/HkFindIntelTbb.txt)

IF(NOT CMAKE_BUILD_TYPE)
SET(CMAKE_BUILD_TYPE RelWithDebInfo CACHE STRING
    "Choose the type of build, options are: None Debug Release RelWithDebInfo MinSizeRel."
    FORCE)
ENDIF(NOT CMAKE_BUILD_TYPE)

OPTION(CONCURRENT "Build concurrent version of hashkat-lib usning threads?" OFF)

OPTION(BUILD_TESTS "Build and install all unit tests ?" ON)

IF(BUILD_TESTS)
    ADD_SUBDIRECTORY(test)
ENDIF(BUILD_TESTS)

















#file(GLOB_RECURSE HASHKAT_LIB_HEADERS 
#    include/*.hxx
#)
#add_custom_target(libhashkat SOURCES ${HASHKAT_LIB_HEADERS})

#add_library(LIBHASHKAT STATIC include/network.h include/action.h)
#set_target_properties(LIBHASHKAT PROPERTIES LINKER_LANGUAGE CXX)

#if (${CMAKE_CXX_COMPILER_ID} STREQUAL MSVC)
#    add_definitions(-DWIN32 -D_CRT_SECURE_NO_WARNINGS -D_SCL_SECURE_NO_WARNINGS /EHa /bigobj)
#endif()

#file(COPY "include" DESTINATION ".")

