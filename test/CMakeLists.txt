IF(BUILD_TESTS)
    FILE(COPY "patterns" DESTINATION ".")
ENDIF(BUILD_TESTS)

INCLUDE_DIRECTORIES(
    "."
    ../include
)

SET(BOOST_ROOT C:/local/boost_1_58_0)
FIND_PACKAGE(Boost 1.50.0)
IF(Boost_FOUND)
    INCLUDE_DIRECTORIES(${Boost_INCLUDE_DIRS})
    ADD_EXECUTABLE(network network.cpp)
    ADD_EXECUTABLE(twitter_follow_action twitter_follow_action.cpp)
    ADD_EXECUTABLE(engine engine.cpp)
    ADD_EXECUTABLE(simulation simulation.cpp)
ENDIF()
