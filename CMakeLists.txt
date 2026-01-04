cmake_minimum_required(VERSION 4.2)
project(cycle LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)

if (CMAKE_BUILD_TYPE STREQUAL "Debug")
    set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
endif()

add_subdirectory(external)

file(GLOB_RECURSE CYCLE_SOURCES "src/*.cpp")
file(GLOB_RECURSE CYCLE_HEADERS "src/*.h")

file(GLOB_RECURSE EXTERNAL_HEADERS "external/stb/*.h" "external/imgui/*.h" "external/cgltf/*.h")
file(GLOB_RECURSE EXTERNAL_SOURCES "external/imgui/*.cpp")

add_executable(engine
    ${CYCLE_SOURCES}
    ${CYCLE_HEADERS}
    ${EXTERNAL_HEADERS}
    ${EXTERNAL_SOURCES}
)

target_include_directories(engine PUBLIC "src/" "external/stb/" "external/cgltf/" "external/imgui/")

target_link_libraries(engine
    SDL3-static
    glm
    volk
    GPUOpen::VulkanMemoryAllocator
	Jolt
)

set(SHADERS_DIR "${CMAKE_CURRENT_SOURCE_DIR}/resources/shaders")

#============= compile shaders =============
file(GLOB_RECURSE GLSL_SOURCE_FILES "${SHADERS_DIR}/*.vert" "${SHADERS_DIR}/*.frag" "${SHADERS_DIR}/*.tesc" "${SHADERS_DIR}/*.tese" "${SHADERS_DIR}/*.comp")

set(GLSL_VALIDATOR "glslangValidator")
foreach(GLSL ${GLSL_SOURCE_FILES})
    get_filename_component(FILE_NAME ${GLSL} NAME)
    set(SPIRV "${SHADERS_DIR}/bin/${FILE_NAME}.spv")
    add_custom_command(
        OUTPUT ${SPIRV}
        COMMAND ${CMAKE_COMMAND} -E make_directory "${SHADERS_DIR}/bin"
        COMMAND ${GLSL_VALIDATOR} -V ${GLSL} -o ${SPIRV}
        DEPENDS ${GLSL})
    list(APPEND SPIRV_BINARY_FILES ${SPIRV})
endforeach(GLSL)

add_custom_target(shaders DEPENDS ${SPIRV_BINARY_FILES})
add_dependencies(engine shaders)
