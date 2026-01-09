cmake_minimum_required(VERSION 3.5)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

project(
    ads-scanner
    VERSION 0.2.0
)

include_directories(include)
find_package(nlohmann_json REQUIRED)

add_executable(
    ads-scanner

    src/kufar.cpp
    src/helperfunctions.cpp
    src/telegram.cpp
    src/networking.cpp
    src/main.cpp
)

target_link_libraries(ads-scanner
    PRIVATE
    curl
)

if (nlohmann_json_FOUND)
    target_link_libraries(ads-scanner PRIVATE nlohmann_json::nlohmann_json)
endif()
