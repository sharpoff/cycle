cmake_minimum_required(VERSION 4.2)
project(cycle LANGUAGES C CXX)

if (CMAKE_BUILD_TYPE STREQUAL "Debug")
    set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
endif()

add_subdirectory(external)

file(GLOB_RECURSE CYCLE_SOURCES "src/*.cpp")
file(GLOB_RECURSE CYCLE_HEADERS "src/*.h")

file(GLOB_RECURSE EXTERNAL_HEADERS "external/stb/*.h" "external/imgui/*.h" "external/cgltf/*.h")
file(GLOB_RECURSE EXTERNAL_SOURCE "external/imgui/*.cpp")

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
)
