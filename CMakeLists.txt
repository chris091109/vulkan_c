cmake_minimum_required(VERSION 3.20)
project(app C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Find required packages
find_package(glfw3 REQUIRED)
find_package(Vulkan REQUIRED)

# Source files
set(SOURCES
    src/main.c
    src/platform.c
    src/vulkan_init.c
    src/rendering.c
)

# Create executable
add_executable(${PROJECT_NAME} ${SOURCES})

# Include directories
target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_SOURCE_DIR}/src
    ${Vulkan_INCLUDE_DIRS}
)

# Link libraries
target_link_libraries(${PROJECT_NAME} PRIVATE
    glfw
    Vulkan::Vulkan
    ${CMAKE_DL_LIBS}
)

# Compiler warnings
if(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(${PROJECT_NAME} PRIVATE
        -Wall
        -Wextra
        -Wpedantic
    )
endif()

# Debug/Release flags
if(CMAKE_BUILD_TYPE MATCHES Debug)
    target_compile_definitions(${PROJECT_NAME} PRIVATE DEBUG)
endif()

# Shader compilation
find_program(GLSLC glslc)
if(GLSLC)
    set(SHADER_DIR ${CMAKE_SOURCE_DIR}/shaders)
    set(SHADER_OUTPUT_DIR ${CMAKE_BINARY_DIR}/shaders)
    
    file(MAKE_DIRECTORY ${SHADER_OUTPUT_DIR})
    
    # Add shader compilation commands
    file(GLOB VERTEX_SHADERS ${SHADER_DIR}/*.vert)
    file(GLOB FRAGMENT_SHADERS ${SHADER_DIR}/*.frag)
    
    foreach(SHADER ${VERTEX_SHADERS} ${FRAGMENT_SHADERS})
        get_filename_component(SHADER_NAME ${SHADER} NAME)
        set(SHADER_OUTPUT ${SHADER_OUTPUT_DIR}/${SHADER_NAME}.spv)
        
        add_custom_command(
            OUTPUT ${SHADER_OUTPUT}
            COMMAND ${GLSLC} ${SHADER} -o ${SHADER_OUTPUT}
            DEPENDS ${SHADER}
            COMMENT "Compiling shader ${SHADER_NAME}"
        )
        
        list(APPEND SHADER_OUTPUTS ${SHADER_OUTPUT})
    endforeach()
    
    add_custom_target(shaders ALL DEPENDS ${SHADER_OUTPUTS})
    add_dependencies(${PROJECT_NAME} shaders)
endif()

