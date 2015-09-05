INCLUDE_DIRECTORIES(
    "."
    ../include
)

IF(WIN32)
    SET(BOOST_ROOT C:/local/boost_1_58_0)
ENDIF(WIN32)
FIND_PACKAGE(Boost 1.50.0)
IF(Boost_FOUND)
    INCLUDE_DIRECTORIES(${Boost_INCLUDE_DIRS})
    ADD_EXECUTABLE(hashkat hashkat.cpp)
    IF(CONCURRENT)
        ADD_EXECUTABLE(hashkat_mt hashkat_mt.cpp)
        IF(TBB_FOUND)
            TARGET_LINK_LIBRARIES(hashkat_mt ${TBB_LIBRARIES})
        ELSE(TBB_FOUND)
            TARGET_LINK_LIBRARIES(hashkat_mt tbb)
        ENDIF(TBB_FOUND)
    ENDIF(CONCURRENT)
ENDIF()
