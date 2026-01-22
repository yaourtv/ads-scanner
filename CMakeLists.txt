cmake_minimum_required(VERSION 3.14)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

project(
    ads-scanner
    VERSION 0.2.0
)

include_directories(include)

# nlohmann/json is header-only; fetch it in a cross-platform way instead of
# relying on system packages on every OS.
include(FetchContent)
FetchContent_Declare(
    nlohmann_json
    GIT_REPOSITORY https://github.com/nlohmann/json.git
    GIT_TAG v3.11.3
)
FetchContent_MakeAvailable(nlohmann_json)

add_executable(
    ads-scanner

    src/kufar.cpp
    src/helperfunctions.cpp
    src/logging.cpp
    src/telegram.cpp
    src/networking.cpp
    src/main.cpp
)

find_package(CURL REQUIRED)

target_link_libraries(ads-scanner
    PRIVATE
        CURL::libcurl
        nlohmann_json::nlohmann_json
)
