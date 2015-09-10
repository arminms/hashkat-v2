INCLUDE_DIRECTORIES(
    "."
    ../include
)

IF(WIN32)
    SET(BOOST_ROOT C:/local/boost_1_59_0)
ENDIF(WIN32)
FIND_PACKAGE(Boost 1.50.0)
IF(Boost_FOUND)
    INCLUDE_DIRECTORIES(${Boost_INCLUDE_DIRS})
    ADD_EXECUTABLE(network network.cpp)
    ADD_EXECUTABLE(twitter_follow_action twitter_follow_action.cpp)
    ADD_EXECUTABLE(engine engine.cpp)
    ADD_EXECUTABLE(simulation_st simulation_st.cpp)
    INSTALL(TARGETS network twitter_follow_action engine simulation_st
        RUNTIME DESTINATION share/hashkat/test)
    IF(CONCURRENT)
        ADD_EXECUTABLE(simulation_mt simulation_mt.cpp)
        IF(TBB_FOUND)
            TARGET_LINK_LIBRARIES(simulation_mt ${TBB_LIBRARIES})
        ELSE(TBB_FOUND)
            TARGET_LINK_LIBRARIES(simulation_mt tbb)
        ENDIF(TBB_FOUND)
    INSTALL(TARGETS simulation_mt RUNTIME DESTINATION share/hashkat/test)
    ENDIF(CONCURRENT)
    INSTALL(DIRECTORY patterns DESTINATION share/hashkat/test)
ENDIF()
