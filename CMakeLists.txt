CMAKE_MINIMUM_REQUIRED(VERSION 2.4 FATAL_ERROR)

PROJECT(hashkat-lib CXX)

#IF(WIN32)
#    SET(USERLIB_PREFIX_DEFAULT "c:/local")
#ELSE(WIN32)
#    SET(USERLIB_PREFIX_DEFAULT "/usr")
#ENDIF(WIN32)

#IF(WIN32)
#    SET(BOOST_PREFIX_DEFAULT "c:/Program Files/Boost")
#    OPTION(BOOST_DYNAMIC "Link to boost DLLs (OFF means static link)" OFF)
#ELSE(WIN32)
#    SET(BOOST_PREFIX_DEFAULT ${USERLIB_PREFIX})
#ENDIF(WIN32)

#INCLUDE(cmake/HkFindBoost.txt)

#INCLUDE_DIRECTORIES(${BOOST_INCLUDE_DIRS})
#LINK_DIRECTORIES(${BOOST_LIB_DIRS})

IF(NOT CMAKE_BUILD_TYPE)
SET(CMAKE_BUILD_TYPE RelWithDebInfo CACHE STRING
    "Choose the type of build, options are: None Debug Release RelWithDebInfo MinSizeRel."
    FORCE)
ENDIF(NOT CMAKE_BUILD_TYPE)

IF(MSVC)
    ADD_DEFINITIONS(-D_CRT_SECURE_NO_WARNINGS -D_SCL_SECURE_NO_WARNINGS)
    SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /EHa /bigobj")
ENDIF()

IF(WIN32 OR APPLE)
ADD_LIBRARY(LIBHASHKAT STATIC
    include/action.h
    include/engine.h
    include/network.h
    include/simulation.h
    include/actions/twitter_add_agent.h
    include/actions/twitter_follow.h
)
SET_TARGET_PROPERTIES(LIBHASHKAT PROPERTIES LINKER_LANGUAGE CXX)
ENDIF(WIN32 OR APPLE)

OPTION(CONCURRENT "Build concurrent version of hashkat-lib usning threads?" ON)
IF(CONCURRENT)
    ADD_DEFINITIONS(-D_CONCURRENT)
    INCLUDE(cmake/HkFindIntelTbb.txt)
    IF(TBB_FOUND)
        TARGET_LINK_LIBRARIES(${TBB_LIBRARIES})
        INCLUDE_DIRECTORIES(${TBB_INCLUDE_DIR})
    ELSE(TBB_FOUND)
        MESSAGE("Building thread version requires Intel TBB library.")
    ENDIF(TBB_FOUND)
ENDIF(CONCURRENT)

OPTION(INSTALL_HEADERS "Install header files ?" ON)
IF(INSTALL_HEADERS)
    FILE(COPY "include" DESTINATION ".")
ENDIF(INSTALL_HEADERS)

OPTION(BUILD_TESTS "Build and install all unit tests ?" ON)
IF(BUILD_TESTS)
    ADD_SUBDIRECTORY(test)
ENDIF(BUILD_TESTS)




















