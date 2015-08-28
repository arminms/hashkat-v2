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
    include_directories(${Boost_INCLUDE_DIRS})
    add_executable(network network.cpp)
    set_property(TARGET network PROPERTY CXX_STANDARD 11)
    add_executable(twitter_follow_action twitter_follow_action.cpp)
    set_property(TARGET twitter_follow_action PROPERTY CXX_STANDARD 11)
    add_executable(engine engine.cpp)
    set_property(TARGET engine PROPERTY CXX_STANDARD 11)
    add_executable(simulation simulation.cpp)
    set_property(TARGET simulation PROPERTY CXX_STANDARD 11)
ENDIF()
