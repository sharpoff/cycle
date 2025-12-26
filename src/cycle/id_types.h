#pragma once

#include <limits>
#include <stdint.h>

enum class EntityID : uint32_t { Invalid = std::numeric_limits<uint32_t>::max() };
enum class ModelID : uint32_t { Invalid = std::numeric_limits<uint32_t>::max() };
enum class MeshID : uint32_t { Invalid = std::numeric_limits<uint32_t>::max() };
enum class TextureID : uint32_t { Invalid = std::numeric_limits<uint32_t>::max() };
enum class MaterialID : uint32_t { Invalid = std::numeric_limits<uint32_t>::max() };

template<typename idType>
inline uint32_t idToUint(idType id)
{
    return uint32_t(id);
}