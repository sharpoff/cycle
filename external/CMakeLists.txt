# sdl
set(SDL_TESTS OFF)
set(SDL_TEST_LIBRARY OFF)
set(SDL_SHARED OFF)
set(SDL_STATIC ON)
set(SDL_DISABLE_INSTALL OFF)
set(SDL_DISABLE_INSTALL_DOCS OFF)
set(SDL_INSTALL_TESTS OFF)
add_subdirectory(SDL)

# glm
set(GLM_BUILD_LIBRARY ON)
set(GLM_BUILD_TESTS OFF)
set(GLM_BUILD_INSTALL OFF)
add_subdirectory(glm)

# volk
add_subdirectory(volk)

# VulkanMemoryAllocator
set(VMA_ENABLE_INSTALL OFF)
add_subdirectory(VulkanMemoryAllocator)

# Jolt Physics
set(CPP_RTTI_ENABLED OFF)
set(CPP_EXCEPTIONS_ENABLED OFF)
set(DOUBLE_PRECISION ON)
set(GENERATE_DEBUG_SYMBOLS ON)
add_subdirectory(JoltPhysics/Build)

# ktx lib
set(KTX_FEATURE_TESTS OFF)
set(KTX_FEATURE_JS OFF)
set(LIBKTX_FEATURE_GL_UPLOAD OFF)
add_subdirectory(KTX-Software)

# soloud + miniaudio backend
file(GLOB_RECURSE SOLOUD_SOURCES "soloud/src/audiosource/*.cpp" "soloud/src/audiosource/*.c" "soloud/src/backend/miniaudio/*.cpp" "soloud/src/core/*.cpp" "soloud/src/filter/*.cpp")
add_library(soloud STATIC ${SOLOUD_SOURCES})
target_include_directories(soloud PUBLIC "soloud/include" "soloud/src/backend/miniaudio/")
target_compile_definitions(soloud PUBLIC WITH_MINIAUDIO)