cmake_minimum_required(VERSION 3.22)
# Set the project Name
project(VulkanWindow)

# Set the C++ Standard to compile against
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Set C++ Flags
set(OPTIMIZE_FLAG "-O2")

if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
	# Clang compiler
	set(DEBUG_FLAGS "-g -ggdb")
elseif (CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
	# GNU compiler
	set(DEBUG_FLAGS "-g -glldb")
endif()

set(WARNING_FLAGS "-Werror -Wpedantic -Wall -Wextra -Wno-zero-as-null-pointer-constant -Wno-c++98-compat-pedantic -Wno-documentation -Wno-documentation-unknown-command -Wno-nonportable-system-include-path -Wno-sign-conversion -Wno-shadow -Wno-old-style-cast -Wno-unused-parameter")

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${OPTIMIZE_FLAG} ${WARNING_FLAGS} ${DEBUG_FLAGS}")

find_package(Vulkan REQUIRED)
find_package(glm REQUIRED)
find_package(glfw3 REQUIRED)

file(COPY src/shaders DESTINATION ${CMAKE_BINARY_DIR})
file(COPY lint_codebase.sh DESTINATION ${CMAKE_BINARY_DIR})
file(COPY run.sh DESTINATION ${CMAKE_BINARY_DIR})

add_executable(VulkanWindow 
	src/main.cpp
	src/triangle_application.cpp
	src/triangle_application.hpp
)

target_link_libraries(${PROJECT_NAME} "-lglfw -lvulkan -ldl -lpthread -lX11 -lXxf86vm -lXrandr -lXi")

