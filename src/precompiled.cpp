#include <stddef.h>
#include <stdint.h>

// vma
#define VK_NO_PROTOTYPES
#define VMA_IMPLEMENTATION
#if 0
#include <core/logger.h>
#define VMA_DEBUG_LOG_FORMAT(format, ...) LOG_PRINTF((format), __VA_ARGS__) // debug allocations
#endif
#include <vk_mem_alloc.h>

// volk
#define VOLK_IMPLEMENTATION
#include <volk.h>

// stb
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

// #define STB_IMAGE_WRITE_IMPLEMENTATION
// #include <stb_image_write.h>

// cgltf
#define CGLTF_IMPLEMENTATION
#include <cgltf.h>

// #define CGLTF_WRITE_IMPLEMENTATION
// #include <cgltf_write.h>