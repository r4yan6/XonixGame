cmake_minimum_required(VERSION 3.16)

project(Xonix LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(SFML 2 COMPONENTS graphics window system REQUIRED)

add_executable(xonix main.cpp)

target_link_libraries(
    xonix
    PRIVATE
    sfml-graphics
    sfml-window
    sfml-system
)

file(
    COPY "${CMAKE_CURRENT_SOURCE_DIR}/images" "${CMAKE_CURRENT_SOURCE_DIR}/assets"
    DESTINATION "${CMAKE_CURRENT_BINARY_DIR}"
)
