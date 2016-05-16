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
        IF(MSVC)
            SET(TBB_PREFIX "C:/local/tbb44_20160413oss")
        ELSE()
            SET(TBB_PREFIX "$ENV{HOME}/tbb44_20160413oss")
        ENDIF()
        INCLUDE(cmake/HkFindIntelTbb.txt)
        IF(TBB_FOUND)
            INCLUDE_DIRECTORIES(BEFORE SYSTEM ${TBB_INCLUDE_DIR})
        ELSE()
            MESSAGE("-- Couldn't find Intel TBB library.")
        ENDIF()
    ENDIF()
ENDIF()

ADD_SUBDIRECTORY(src)

OPTION(INSTALL_HEADERS "Install headers ?" ON)
IF(INSTALL_HEADERS)
    ADD_SUBDIRECTORY(include)
ENDIF()

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

# build a CPack driven installer package
INCLUDE(InstallRequiredSystemLibraries)
SET(CPACK_PACKAGE_NAME "Hashkat")
SET(CPACK_PACKAGE_VENDOR "hashkat.org")
SET(CPACK_PACKAGE_DESCRIPTION_FILE "${CMAKE_SOURCE_DIR}/readme.txt")
SET(CPACK_PACKAGE_DESCRIPTION_SUMMARY
    "Hashkat - A dynamic network simulation tool designed to model the growth of and information propagation through an online social network.")
SET(CPACK_RESOURCE_FILE_LICENSE
    "${CMAKE_SOURCE_DIR}/license.txt")
SET(CPACK_PACKAGE_VERSION_MAJOR "${HASHKAT_VERSION_MAJOR}")
SET(CPACK_PACKAGE_VERSION_MINOR "${HASHKAT_VERSION_MINOR}")
SET(CPACK_PACKAGE_VERSION_PATCH "${HASHKAT_VERSION_PATCH}")
SET(CPACK_PACKAGE_CONTACT "asobhani@sharcnet.ca")
SET(CPACK_COMPONENTS_ALL hashkat hashkat_mt headers tests patterns)
SET(CPACK_COMPONENT_HASHKAT_DISPLAY_NAME "hashkat")
SET(CPACK_COMPONENT_HASHKAT_MT_DISPLAY_NAME "hashkat_mt")
SET(CPACK_COMPONENT_HEADERS_DISPLAY_NAME "C++ Headers")
SET(CPACK_COMPONENT_TESTS_DISPLAY_NAME "Unit Tests")
SET(CPACK_COMPONENT_PATTERNS_DISPLAY_NAME "Pattern Files")
SET(CPACK_COMPONENT_HASHKAT_DESCRIPTION 
    "Single-threaded version of Hashkat")
SET(CPACK_COMPONENT_HASHKAT_MT_DESCRIPTION 
    "Multi-threaded version of Hashkat")
SET(CPACK_COMPONENT_HEADERS_DESCRIPTION 
    "Header files in case you like to develop using Hashkat")
SET(CPACK_COMPONENT_TESTS_DESCRIPTION
    "Binaries for unit testing")
SET(CPACK_COMPONENT_PATTERNS_DESCRIPTION
    "Pattern files required by unit tests")
SET(CPACK_COMPONENT_PATTERNS_DEPENDS tests)
SET(CPACK_COMPONENT_HASHKAT_GROUP "Program")
SET(CPACK_COMPONENT_HASHKAT_MT_GROUP "Program")
SET(CPACK_COMPONENT_HEADERS_GROUP "Development")
SET(CPACK_COMPONENT_TESTS_GROUP "Testing")
SET(CPACK_COMPONENT_PATTERNS_GROUP "Testing")
SET(CPACK_COMPONENT_GROUP_PROGRAM_DESCRIPTION
    "Main binaries")
SET(CPACK_COMPONENT_GROUP_DEVELOPMENT_DESCRIPTION
    "All you need to start developing using Hashkat")
SET(CPACK_COMPONENT_GROUP_TESTING_DESCRIPTION
    "Supporting files for unit testing")
INCLUDE(CPack)
