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
    TARGET_LINK_LIBRARIES(hashkat boost_program_options)
    INSTALL(TARGETS hashkat RUNTIME DESTINATION bin)
    IF(CONCURRENT)
        ADD_EXECUTABLE(hashkat_mt hashkat_mt.cpp)
        IF(TBB_FOUND)
            TARGET_LINK_LIBRARIES(hashkat_mt ${TBB_LIBRARIES} boost_program_options)
        ELSE(TBB_FOUND)
            TARGET_LINK_LIBRARIES(hashkat_mt tbb boost_program_options)
        ENDIF(TBB_FOUND)
    INSTALL(TARGETS hashkat_mt RUNTIME DESTINATION bin)
    ENDIF(CONCURRENT)
ENDIF()
