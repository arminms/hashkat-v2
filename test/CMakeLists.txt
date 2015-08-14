file(COPY "patterns" DESTINATION ".")

include_directories(
    "."
    ../include
)

set(BOOST_ROOT C:/local/boost_1_58_0)
#set(Boost_USE_STATIC_LIBS        ON)
#set(Boost_USE_MULTITHREADED      ON)
#set(Boost_USE_STATIC_RUNTIME    OFF)
find_package(Boost 1.50.0)
 if(Boost_FOUND)
   include_directories(${Boost_INCLUDE_DIRS})
#   LINK_DIRECTORIES(${Boost_LIBRARY_DIRS})
   add_executable(network network.cpp)
#   target_link_libraries(linking_with_boost ${Boost_LIBRARIES})
 endif()
