cmake_minimum_required(VERSION 3.21)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
if (APPLE)
    set(CMAKE_OSX_ARCHITECTURES "arm64;x86_64")
endif()
set(CMAKE_CXX_VISIBILITY_PRESET hidden)

project(object-workshop VERSION 1.0.0)

file(GLOB SOURCES
	src/*.cpp
	src/nodes/*.cpp
	src/ui/*.cpp
	src/ui/popups/*.cpp
	src/ui/admin/*.cpp
	src/ui/auth/*.cpp

    src/gdauth/*.cpp # until fig ACCEPTS MY PR, for some reason hes inactive...
)

# Set up the mod binary
add_library(${PROJECT_NAME} SHARED ${SOURCES})

if (NOT DEFINED ENV{GEODE_SDK})
    message(FATAL_ERROR "Unable to find Geode SDK! Please define GEODE_SDK environment variable to point to Geode")
else()
    message(STATUS "Found Geode: $ENV{GEODE_SDK}")
endif()

add_subdirectory($ENV{GEODE_SDK} ${CMAKE_CURRENT_BINARY_DIR}/geode)

CPMAddPackage("gh:FireMario211/dashauth#main")
target_link_libraries(${PROJECT_NAME} DashAuth)

setup_geode_mod(${PROJECT_NAME})
