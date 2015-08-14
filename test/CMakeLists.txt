file(COPY "patterns" DESTINATION ".")

include_directories(
    "."
    ../include
)

find_package(Boost 1.50.0)
 if(Boost_FOUND)
   include_directories(${Boost_INCLUDE_DIRS})
   add_executable(network network.cpp)
 endif()
