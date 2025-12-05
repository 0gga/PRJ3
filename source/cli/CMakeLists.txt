get_filename_component(PROJ_NAME ${CMAKE_CURRENT_SOURCE_DIR} NAME)
project(${PROJ_NAME} LANGUAGES CXX)

# Collect all .c, .cpp, .h, .tpp files from src/ & include/
file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS *.cpp *.c *.tpp *.hpp *.h)

# Define executable path
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/bin)

# Create the executable
add_executable(${PROJECT_NAME} ${SOURCES})

add_subdirectory(${PROJECT_SOURCE_DIR}/client/pn532-lib)

target_link_libraries(cli PRIVATE pn532_lib)

# Set C++ standard
set_property(TARGET ${PROJECT_NAME} PROPERTY CXX_STANDARD 23)
set_property(TARGET ${PROJECT_NAME} PROPERTY CXX_STANDARD_REQUIRED ON)