IF(BUILD_TESTS)
    FILE(COPY "patterns" DESTINATION ".")
ENDIF(BUILD_TESTS)

INCLUDE_DIRECTORIES(
    "."
    ../include
)

IF(WIN32)
    SET(BOOST_ROOT C:/local/boost_1_58_0)
END(WIN32)
FIND_PACKAGE(Boost 1.50.0)
IF(Boost_FOUND)
    INCLUDE_DIRECTORIES(${Boost_INCLUDE_DIRS})
    ADD_EXECUTABLE(network network.cpp)
    ADD_EXECUTABLE(twitter_follow_action twitter_follow_action.cpp)
    ADD_EXECUTABLE(engine engine.cpp)
    ADD_EXECUTABLE(simulation simulation.cpp)
    IF(CONCURRENT)
        TARGET_LINK_LIBRARIES(network ${TBB_LIBRARIES})
        TARGET_LINK_LIBRARIES(twitter_follow_action ${TBB_LIBRARIES})
        TARGET_LINK_LIBRARIES(engine ${TBB_LIBRARIES})
        TARGET_LINK_LIBRARIES(simulation ${TBB_LIBRARIES})
    ENDIF(CONCURRENT)
ENDIF()
