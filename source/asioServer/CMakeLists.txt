cmake_minimum_required(VERSION 3.27)
get_filename_component(PROJ_NAME ${CMAKE_CURRENT_SOURCE_DIR} NAME)
project(${PROJ_NAME} LANGUAGES C CXX)

# Collect all .c, .cpp, .h, .tpp files from src/ & include/
file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS src/*.cpp src/*.c src/*.tpp)
file(GLOB_RECURSE HEADERS CONFIGURE_DEPENDS include/*.cpp include/*.c include/*.tpp include/*.hpp include/*.h)
file(GLOB_RECURSE SERVER CONFIGURE_DEPENDS TCP/*.cpp TCP/*.c TCP/*.hpp TCP/*.h TCP/*.tpp)

# Define executable path
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/bin)

# Create the executable
add_executable(${PROJECT_NAME} ${SOURCES} ${HEADERS} ${SERVER})

# Include headers from include/
target_include_directories(${PROJECT_NAME} PRIVATE ${PROJECT_SOURCE_DIR}/include PRIVATE ${PROJECT_SOURCE_DIR}/TCP)

# Include nlohmann::json
include_directories(${CMAKE_SOURCE_DIR}/extern/json/single_include/nlohmann)

########################## Boost ##########################
include_directories(
${CMAKE_SOURCE_DIR}/extern/asio/include
${CMAKE_SOURCE_DIR}/extern/system/include
${CMAKE_SOURCE_DIR}/extern/config/include
${CMAKE_SOURCE_DIR}/extern/assert/include
${CMAKE_SOURCE_DIR}/extern/core/include
${CMAKE_SOURCE_DIR}/extern/throw_exception/include
${CMAKE_SOURCE_DIR}/extern/align/include
${CMAKE_SOURCE_DIR}/extern/static_assert/include
${CMAKE_SOURCE_DIR}/extern/type_traits/include
${CMAKE_SOURCE_DIR}/extern/detail/include
${CMAKE_SOURCE_DIR}/extern/winapi/include
${CMAKE_SOURCE_DIR}/extern/predef/include
${CMAKE_SOURCE_DIR}/extern/integer/include
${CMAKE_SOURCE_DIR}/extern/utility/include
${CMAKE_SOURCE_DIR}/extern/move/include
${CMAKE_SOURCE_DIR}/extern/type_index/include
)

if (WIN32)
	target_link_libraries(${PROJECT_NAME} PRIVATE ws2_32 wsock32)
elseif (UNIX)
	target_link_libraries(${PROJECT_NAME} PRIVATE pthread)
endif()

target_compile_definitions(${PROJECT_NAME} PRIVATE BOOST_ERROR_CODE_HEADER_ONLY)
########################## Boost ##########################

# Set C++ standard
set_property(TARGET ${PROJECT_NAME} PROPERTY CXX_STANDARD 23)
set_property(TARGET ${PROJECT_NAME} PROPERTY CXX_STANDARD_REQUIRED ON)