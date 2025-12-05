get_filename_component(PROJ_NAME ${CMAKE_CURRENT_SOURCE_DIR} NAME)
project(${PROJ_NAME} LANGUAGES CXX)

# Collect all .c, .cpp, .h, .tpp files from src/ & include/
file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS *.cpp *.c *.tpp *.hpp *.h)
file(GLOB_RECURSE PN532 CONFIGURE_DEPENDS
        ${CMAKE_CURRENT_SOURCE_DIR}/pn532-lib/*.c
        ${CMAKE_CURRENT_SOURCE_DIR}/pn532-lib/*.h
        ${CMAKE_CURRENT_SOURCE_DIR}/pn532-lib/rpi_tcptest/*.h
        ${CMAKE_CURRENT_SOURCE_DIR}/pn532-lib/rpi_tcptest/*.cpp
)

# Define executable path
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/bin)

# Create the executable
add_executable(${PROJECT_NAME} ${SOURCES} ${PN532})

target_include_directories(${PROJECT_NAME} PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/pn532-lib
        ${CMAKE_CURRENT_SOURCE_DIR}/pn532-lib/rpi_tcptest
)

if (UNIX)
	target_link_libraries(${PROJECT_NAME} PRIVATE wiringPi PRIVATE pthread rt)
endif()


# Set C++ standard
set_property(TARGET ${PROJECT_NAME} PROPERTY CXX_STANDARD 23)
set_property(TARGET ${PROJECT_NAME} PROPERTY CXX_STANDARD_REQUIRED ON)