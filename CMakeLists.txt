CMAKE_MINIMUM_REQUIRED(VERSION 2.6 FATAL_ERROR)

#
# If the user specifies -DCMAKE_BUILD_TYPE on the command line, take their
# definition and dump it in the cache along with proper documentation,
# otherwise set CMAKE_BUILD_TYPE to Release prior to calling PROJECT()
#
IF(DEFINED CMAKE_BUILD_TYPE)
    SET(CMAKE_BUILD_TYPE ${CMAKE_BUILD_TYPE} CACHE STRING
        "Choose the type of build, options are: None(CMAKE_CXX_FLAGS or
         CMAKE_C_FLAGS used) Debug Release RelWithDebInfo MinSizeRel.")
ELSE()
    SET(CMAKE_BUILD_TYPE Release CACHE STRING
        "Choose the type of build, options are: None(CMAKE_CXX_FLAGS or
         CMAKE_C_FLAGS used) Debug Release RelWithDebInfo MinSizeRel.")
ENDIF()

PROJECT(hashkat-v2 CXX)

# the version number must be set only here
SET(HASHKAT_VERSION_MAJOR "2")
SET(HASHKAT_VERSION_MINOR "0")
SET(HASHKAT_VERSION_PATCH "0")
SET(HASHKAT_VERSION_TWEAK "0")

SET(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)

IF(MSVC)
    ADD_DEFINITIONS(-D_CRT_SECURE_NO_WARNINGS -D_SCL_SECURE_NO_WARNINGS)
    SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /EHa /bigobj")
ELSE()
    INCLUDE(CheckCXXCompilerFlag)
    CHECK_CXX_COMPILER_FLAG("-std=c++11" COMPILER_SUPPORTS_CXX11)
    IF(NOT COMPILER_SUPPORTS_CXX11) 
       CHECK_CXX_COMPILER_FLAG("-std=c++0x" COMPILER_SUPPORTS_CXX0X)
    ENDIF()
    IF(COMPILER_SUPPORTS_CXX11)
        SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
    ELSEIF(COMPILER_SUPPORTS_CXX0X)
        SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++0x")
    ELSE()
        MESSAGE(STATUS "The compiler ${CMAKE_CXX_COMPILER} has no C++11 support. Please use a different C++ compiler.")
    ENDIF()
ENDIF()

OPTION(CONCURRENT "Build concurrent version of hashkat-lib usning threads?" OFF)
IF(CONCURRENT)
    OPTION(FIND_INTEL_TBB "Try to find Intel TBB library ?" OFF)
    SET(TBB_FOUND FALSE)
    IF(FIND_INTEL_TBB)
        IF(WIN32)
            SET(TBB_PREFIX "C:/local/tbb44_20160413oss")
        ELSE(WIN32)
            SET(TBB_PREFIX "$ENV{HOME}/tbb44_20160413oss")
        ENDIF(WIN32)
        INCLUDE(cmake/HkFindIntelTbb.txt)
        IF(TBB_FOUND)
            INCLUDE_DIRECTORIES(BEFORE SYSTEM ${TBB_INCLUDE_DIR})
        ELSE(TBB_FOUND)
            MESSAGE("-- Couldn't find Intel TBB library.")
        ENDIF(TBB_FOUND)
    ENDIF(FIND_INTEL_TBB)
ENDIF(CONCURRENT)

OPTION(INSTALL_HEADERS "Install headers ?" ON)
IF(INSTALL_HEADERS)
    ADD_SUBDIRECTORY(include)
ENDIF(INSTALL_HEADERS)

# testing
OPTION(BUILD_TESTS "Build unit tests ?" OFF)
IF(BUILD_TESTS)
    ADD_SUBDIRECTORY(test)
    ENABLE_TESTING()
    ADD_TEST(NAME network COMMAND test_network
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/test)
    ADD_TEST(NAME twitter_follow_action COMMAND test_twitter_follow_action
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/test)
    ADD_TEST(NAME engine COMMAND test_engine
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/test)
    ADD_TEST(NAME simulation_st COMMAND test_simulation_st
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/test)
    IF(CONCURRENT)
        ADD_TEST(NAME simulation_mt COMMAND test_simulation_mt
            WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/test)
    ENDIF()
ENDIF()
OPTION(INSTALL_TESTS "Install unit tests ?" OFF)

INSTALL(FILES src/INFILE.xml DESTINATION share/hashkat)

ADD_SUBDIRECTORY(src)
