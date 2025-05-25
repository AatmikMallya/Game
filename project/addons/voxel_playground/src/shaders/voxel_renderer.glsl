#[compute]
#version 460

#include "voxel_world.glsl"

// ----------------------------------- STRUCTS -----------------------------------

struct Light {
    vec4 position;
    vec4 color;
};

struct Ray {
    vec3 d;
    vec3 o;
    vec3 rD;
};


// ----------------------------------- GENERAL STORAGE -----------------------------------

layout(set = 1, binding = 0, rgba8) restrict uniform writeonly image2D outputImage;
layout(set = 1, binding = 1, r32f) restrict uniform writeonly image2D depthBuffer;

layout(std430, set = 1, binding = 2) restrict buffer Params {
    vec4 background; //rgb, brightness
    int width;
    int height;
    float fov;
    uint triangleCount;
    uint blas_count;
} params;

layout(std430, set = 1, binding = 3) restrict buffer Camera {
    mat4 view_projection;
    mat4 inv_view_projection;
    vec4 position;
    uint frame_index;
    float near;
    float far;
    float padding;
} camera;

// ----------------------------------- FUNCTIONS -----------------------------------


vec2 pcg2d(inout uvec2 seed) {
	// PCG2D, as described here: https://jcgt.org/published/0009/03/02/
	seed = 1664525u * seed + 1013904223u;
	seed.x += 1664525u * seed.y;
	seed.y += 1664525u * seed.x;
	seed ^= (seed >> 16u);
	seed.x += 1664525u * seed.y;
	seed.y += 1664525u * seed.x;
	seed ^= (seed >> 16u);
	// Multiply by 2^-32 to get floats
	return vec2(seed) * 2.32830643654e-10; 
}

uvec2 prng_seed(vec2 pos, uint frame) {
    uvec2 seed = uvec2(pos.xy);
    seed = seed * 0x9e3779b9u + uvec2(frame);
    seed ^= seed >> 16u;
    return seed * 0x9e3779b9u;
}

vec2 box_muller(vec2 rands) {
    float R = sqrt(-2.0f * log(rands.x));
    float theta = 6.2831853f * rands.y;
    return vec2(cos(theta), sin(theta));
}

vec3 sampleSky(const vec3 direction) {
    float t = 0.5 * (direction.y + 1.0);
    return mix(vec3(0.95), vec3(0.9, 0.94, 1.0), t) * 1.0f;
}

vec3 sampleMiddleSlice(vec2 screen_uv) {
    int x = int(floor(screen_uv.x * float(voxelWorldProperties.grid_size.x)));
    int y = int(floor(screen_uv.y * float(voxelWorldProperties.grid_size.y)));
    int z = voxelWorldProperties.grid_size.z / 4;
    
    x = clamp(x, 0, voxelWorldProperties.grid_size.x - 1);
    y = clamp(y, 0, voxelWorldProperties.grid_size.y - 1);
    z = clamp(z, 0, voxelWorldProperties.grid_size.z - 1);
    
    
    uint index = posToIndex(ivec3(x, y, z));

    uint voxelValue = voxelData[index].data;    
    float intensity = float(voxelValue);    
    return vec3(intensity);
}

layout(local_size_x = 32, local_size_y = 32, local_size_z = 1) in;
void main() {
    ivec2 pos = ivec2(gl_GlobalInvocationID.xy);
    if (pos.x >= params.width || pos.y >= params.height) return;

    uvec2 seed = prng_seed(gl_GlobalInvocationID.xy, camera.frame_index);
    vec2 screen_uv = vec2(pos + 0.5) / vec2(params.width, params.height); // + box_muller(pcg2d(seed) * 0.25)

    vec4 ndc = vec4(screen_uv * 2.0 - 1.0, 0.0, 1.0);
    ndc.y = -ndc.y;

    vec4 world_pos = inverse(camera.view_projection) * ndc;
    world_pos /= world_pos.w;
    vec3 ray_origin = camera.position.xyz;
    vec3 ray_dir = normalize(world_pos.xyz - ray_origin);

    ivec3 grid_position;
    vec3 normal;
    int step_count = 0;
    float t = voxelRaytrace(ray_origin, ray_dir, vec2(camera.near, camera.far), grid_position, normal, step_count);
    vec3 color = vec3(0.0);
    if (t >= 0.0) {
        vec3 hitPos = ray_origin + t * ray_dir;
        vec3 baseColor = vec3(grid_position) / voxelWorldProperties.grid_size.xyz;
        color = baseColor;
        color *= 0.2 * dot(normal, vec3(0.25, 0.35, 0.4)) + 0.8;
        
    } else {
        color = sampleSky(ray_dir);
    }


    // color = vec3(step_count) * 0.01;

    // color = sampleMiddleSlice(screen_uv);

    // depth = camera.far / (camera.far - camera.near) * (1.0 - camera.near / depth);
    float depth = 0.0f;
    imageStore(outputImage, pos, vec4(color, 1.0));
    imageStore(depthBuffer, pos, vec4(depth, 0.0, 0.0, 0.0));
}