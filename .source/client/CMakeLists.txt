cmake_minimum_required(VERSION 3.27)
get_filename_component(PROJ_NAME ${CMAKE_CURRENT_SOURCE_DIR} NAME)
project(${PROJ_NAME} LANGUAGES CXX)

# Collect all .c, .cpp, .h, .tpp files from src/ & include/
file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS pn532-lib/*.cpp pn532-lib/*.c pn532-lib/*.tpp pn532-lib/*.hpp pn532-lib/*.h)

# Define executable path
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/bin)

# Create the executable
add_executable(${PROJECT_NAME} ${SOURCES})

# Include nlohmann::json
include_directories(${CMAKE_SOURCE_DIR}/extern/json/single_include/nlohmann)

# Set C++ standard
set_property(TARGET ${PROJECT_NAME} PROPERTY CXX_STANDARD 23)
set_property(TARGET ${PROJECT_NAME} PROPERTY CXX_STANDARD_REQUIRED ON)