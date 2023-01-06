cmake_minimum_required(VERSION 3.10)
project(main CXX)

# ---------------------------------------------------------------------------------------
# Example of using pre-compiled library
# ---------------------------------------------------------------------------------------

set(root ${CMAKE_CURRENT_LIST_DIR})

link_directories(
${root}/lib
)

add_executable(main main.cpp)

target_link_libraries(main PRIVATE pthread spdlog)

target_include_directories(
main
PUBLIC
${root}/include
)



