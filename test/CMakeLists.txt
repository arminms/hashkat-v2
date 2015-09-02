IF(BUILD_TESTS AND COPY_PATTERNS)
    FILE(COPY "patterns" DESTINATION ".")
ENDIF(BUILD_TESTS AND COPY_PATTERNS)

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
    ADD_EXECUTABLE(network network.cpp)
    ADD_EXECUTABLE(twitter_follow_action twitter_follow_action.cpp)
    ADD_EXECUTABLE(engine engine.cpp)
    ADD_EXECUTABLE(simulation_st simulation_st.cpp)
    IF(CONCURRENT)
        ADD_EXECUTABLE(simulation_mt simulation_mt.cpp)
        TARGET_LINK_LIBRARIES(network ${TBB_LIBRARIES})
        TARGET_LINK_LIBRARIES(twitter_follow_action ${TBB_LIBRARIES})
        TARGET_LINK_LIBRARIES(engine ${TBB_LIBRARIES})
        TARGET_LINK_LIBRARIES(simulation_mt ${TBB_LIBRARIES})
    ENDIF(CONCURRENT)
ENDIF()
