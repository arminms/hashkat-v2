file(COPY "patterns" DESTINATION ".")

include_directories(
    "."
    ../include
)

set(BOOST_ROOT C:/local/boost_1_58_0)
find_package(Boost 1.50.0)
if(Boost_FOUND)
	include_directories(${Boost_INCLUDE_DIRS})
	add_executable(network network.cpp)
	set_property(TARGET network PROPERTY CXX_STANDARD 11)
	add_executable(twitter_follow_action twitter_follow_action.cpp)
	set_property(TARGET twitter_follow_action PROPERTY CXX_STANDARD 11)
	add_executable(engine engine.cpp)
	set_property(TARGET engine PROPERTY CXX_STANDARD 11)
	add_executable(simulation simulation.cpp)
	set_property(TARGET simulation PROPERTY CXX_STANDARD 11)
endif()
