cmake_minimum_required(VERSION 4.1)
project(GenealogyGraphCreator)

set(CMAKE_CXX_STANDARD 20)
find_package(SDL3 REQUIRED)
find_package(SDL3_ttf REQUIRED)

add_executable(GenealogyGraphCreator
        main.cpp)

target_include_directories(GenealogyGraphCreator PRIVATE
        /usr/include/SDL3_gfx
)

target_link_libraries(${PROJECT_NAME} PRIVATE SDL3::SDL3 SDL3_ttf::SDL3_ttf SDL3_gfx)