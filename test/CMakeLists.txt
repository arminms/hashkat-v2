INCLUDE_DIRECTORIES(
    "."
    ../include
)

IF(MSVC)
    SET(BOOST_ROOT C:/local/boost_1_59_0)
ENDIF()
FIND_PACKAGE(Boost 1.50.0)
IF(Boost_FOUND)
    INCLUDE_DIRECTORIES(${Boost_INCLUDE_DIRS})
    ADD_EXECUTABLE(test_network network.cpp)
    ADD_EXECUTABLE(test_twitter_follow_action twitter_follow_action.cpp)
    ADD_EXECUTABLE(test_engine engine.cpp)
    ADD_EXECUTABLE(test_simulation_st simulation_st.cpp)
    IF(MSVC)
        INSTALL(TARGETS
            test_network
            test_twitter_follow_action
            test_engine
            test_simulation_st
            RUNTIME DESTINATION test COMPONENT tests)
        IF(CONCURRENT)
            ADD_EXECUTABLE(test_simulation_mt simulation_mt.cpp)
            IF(TBB_FOUND)
                TARGET_LINK_LIBRARIES(test_simulation_mt ${TBB_LIBRARIES})
            ELSE()
                TARGET_LINK_LIBRARIES(test_simulation_mt tbb)
            ENDIF()
            INSTALL(TARGETS test_simulation_mt
                    RUNTIME DESTINATION test
                    COMPONENT tests)
        ENDIF()
        INSTALL(DIRECTORY patterns DESTINATION test COMPONENT patterns)
    ELSE()
        INSTALL(TARGETS
            test_network
            test_twitter_follow_action
            test_engine
            test_simulation_st
            RUNTIME DESTINATION share/hashkat/test COMPONENT tests)
        IF(CONCURRENT)
            ADD_EXECUTABLE(test_simulation_mt simulation_mt.cpp)
            IF(TBB_FOUND)
                TARGET_LINK_LIBRARIES(test_simulation_mt ${TBB_LIBRARIES})
            ELSE()
                TARGET_LINK_LIBRARIES(test_simulation_mt tbb)
            ENDIF()
            INSTALL(TARGETS test_simulation_mt
                    RUNTIME DESTINATION share/hashkat/test
                    COMPONENT tests)
        ENDIF()
        INSTALL(DIRECTORY patterns
                DESTINATION share/hashkat/test
                COMPONENT patterns)
    ENDIF()
    FILE(COPY patterns DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
ENDIF()
