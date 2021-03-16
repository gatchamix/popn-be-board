# CMakeList.txt : Top-level CMake project file, do global configuration
# and include sub-projects here.
#
cmake_minimum_required (VERSION 3.13)

project ("popn be-board")

set(CMAKE_CXX_STANDARD 20)
add_definitions(-DUNICODE -D_UNICODE)

# Include sub-projects.
add_subdirectory ("minhook")
add_subdirectory ("hook")
add_subdirectory ("inject")

add_dependencies ("hook" "minhook")

target_link_libraries("hook" PRIVATE "minhook")