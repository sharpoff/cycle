#pragma once

#include <limits>
#include <stdint.h>

enum class EntityID : uint32_t { Invalid = std::numeric_limits<uint32_t>::max() };
enum class ModelID : uint32_t { Invalid = std::numeric_limits<uint32_t>::max() };
enum class TextureID : uint32_t { Invalid = std::numeric_limits<uint32_t>::max() };
enum class MaterialID : uint32_t { Invalid = std::numeric_limits<uint32_t>::max() };

enum class RenderPassID : uint32_t { Invalid = std::numeric_limits<uint32_t>::max() };