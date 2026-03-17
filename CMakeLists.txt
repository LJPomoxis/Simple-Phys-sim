cmake_minimum_required(VERSION 3.10)
project(MySFMLProject)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(project_name "sfmlTestProj")

include(FetchContent)

FetchContent_Declare(
    SFML
    GIT_REPOSITORY https://github.com/SFML/SFML.git
    GIT_TAG 3.0.2
)

# This ensures SFML is built by YOUR compiler during your project's build
FetchContent_MakeAvailable(SFML)

add_executable(${project_name} main.cpp)

# Link against the targets (CMake handles the DLL copying for you in SFML 3)
target_link_libraries(${project_name} PRIVATE sfml-graphics sfml-window sfml-system)