#[compute]
#version 460

#include "utility.glsl"

// Shared inputs (match voxel_renderer.glsl)
layout(std430, set = 1, binding = 2) restrict buffer Params {
    vec4 background;
    int width;
    int height;
    float fov;
} params;

layout(std430, set = 1, binding = 3) restrict buffer Camera {
    mat4 view_projection;
    mat4 inv_view_projection;
    vec4 position;
    uint frame_index;
    float near;
    float far;
} camera;

// Entities
#ifndef MAX_ENTITIES
#define MAX_ENTITIES 32
#endif

struct Brick {
    uint occupancy_count;
    uint voxel_data_pointer;
};

struct Voxel {
    uint data;
};

#define BRICK_VOLUME 512u
bool isVoxelAir(Voxel v) { return ((v.data >> 24) & 0xFFu) == 0u; }

struct EntityDescriptor {
    mat4 local_to_world;
    mat4 world_to_local;
    vec4 aabb_min;
    vec4 aabb_max;
    vec4 grid_size;
    vec4 brick_grid_size;
    float scale;
    uint brick_offset;
    uint voxel_offset;
    uint brick_count;
    uint enabled;
    float health;
    float _pad0; float _pad1; float _pad2;
};

layout(std430, set = 1, binding = 6) restrict buffer EntityCount {
    int entity_count;
} entityCount;

layout(std430, set = 1, binding = 9) restrict buffer EntityDescriptors {
    EntityDescriptor entities[MAX_ENTITIES];
} entityDesc;

layout(std430, set = 1, binding = 7) restrict buffer EntityBricksBuf {
    Brick entityBricks[];
};

layout(std430, set = 1, binding = 8) restrict buffer EntityVoxelsBuf {
    Voxel entityVoxels[];
};

bool entityIsValidPos(ivec3 pos, int id) {
    ivec3 gs = ivec3(entityDesc.entities[id].grid_size.xyz);
    return pos.x >= 0 && pos.x < gs.x && pos.y >= 0 && pos.y < gs.y && pos.z >= 0 && pos.z < gs.z;
}

uint entityGetBrickIndex(ivec3 pos, int id) {
    ivec3 bgs = ivec3(entityDesc.entities[id].brick_grid_size.xyz);
    ivec3 brickCoord = pos / 8;
    return uint(brickCoord.x + brickCoord.y * bgs.x + brickCoord.z * bgs.x * bgs.y);
}

#define USE_MORTON_ORDER
uint entityGetVoxelIndexInBrick(ivec3 pos) {
    ivec3 localPos = pos % 8;
#ifdef USE_MORTON_ORDER
    uint morton = 0u;
    morton |= ((uint(localPos.x) >> 0) & 1u) << 0;
    morton |= ((uint(localPos.y) >> 0) & 1u) << 1;
    morton |= ((uint(localPos.z) >> 0) & 1u) << 2;
    morton |= ((uint(localPos.x) >> 1) & 1u) << 3;
    morton |= ((uint(localPos.y) >> 1) & 1u) << 4;
    morton |= ((uint(localPos.z) >> 1) & 1u) << 5;
    morton |= ((uint(localPos.x) >> 2) & 1u) << 6;
    morton |= ((uint(localPos.y) >> 2) & 1u) << 7;
    morton |= ((uint(localPos.z) >> 2) & 1u) << 8;
    return morton;
#endif
    return uint(localPos.x + (localPos.y * 8) + (localPos.z * 64));
}

Voxel entityGetVoxel(uint index) { return entityVoxels[index]; }

// Output per-pixel: (t, normal.xyz). t = large if no hit
layout(set = 1, binding = 0, rgba32f) uniform writeonly image2D entityResult;

layout(local_size_x = 32, local_size_y = 32, local_size_z = 1) in;
void main() {
    ivec2 pos = ivec2(gl_GlobalInvocationID.xy);
    if (pos.x >= params.width || pos.y >= params.height) return;

    vec2 screen_uv = vec2(pos + 0.5) / vec2(params.width, params.height);
    vec4 ndc = vec4(screen_uv * 2.0 - 1.0, 0.0, 1.0);
    ndc.y = -ndc.y;
    vec4 world_pos = inverse(camera.view_projection) * ndc;
    world_pos /= world_pos.w;
    vec3 ro_ws = camera.position.xyz;
    vec3 rd_ws = normalize(world_pos.xyz - ro_ws);

    // Debug gradient write to validate pass wiring; no SSBO reads.
    vec3 dbg = vec3(screen_uv, 0.0);
    imageStore(entityResult, pos, vec4(dbg, 1.0));
}
