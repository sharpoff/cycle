set(SDL_TESTS OFF)
set(SDL_TEST_LIBRARY OFF)
set(SDL_SHARED OFF)
set(SDL_STATIC ON)
set(SDL_DISABLE_INSTALL OFF)
set(SDL_DISABLE_INSTALL_DOCS OFF)
set(SDL_INSTALL_TESTS OFF)
add_subdirectory(SDL)

set(GLM_BUILD_LIBRARY ON)
set(GLM_BUILD_TESTS OFF)
set(GLM_BUILD_INSTALL OFF)
add_subdirectory(glm)

add_subdirectory(volk)

set(VMA_ENABLE_INSTALL OFF)
add_subdirectory(VulkanMemoryAllocator)

set(CPP_RTTI_ENABLED OFF)
add_subdirectory(JoltPhysics/Build)
