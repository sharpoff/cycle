#ifndef CONSTANTS_GLSL
#define CONSTANTS_GLSL

#define PI 3.1415

// Rendering flags
const uint RENDERING_NONE = 0;
const uint RENDERING_FILL = 1;
const uint RENDERING_WIREFRAME = 2;
const uint RENDERING_LIGHTING = 1 << 1;
const uint RENDERING_SHADOWS = 1 << 2;
const uint RENDERING_MATERIAL = 1 << 3;
const uint RENDERING_ALL = RENDERING_FILL | RENDERING_LIGHTING | RENDERING_SHADOWS | RENDERING_MATERIAL;

const int SHADOWMAP_CASCADES = 4;

const int INVALID_ID = 4294967295;

// Light types
const uint LIGHT_TYPE_DIRECTIONAL = 0;
const uint LIGHT_TYPE_POINT = 1;
const uint LIGHT_TYPE_SPOT = 2;

const int SAMPLER_LINEAR_ID = 0;
const int SAMPLER_NEAREST_ID = 1;

#endif // #define CONSTANTS_GLSL