#[compute]
#version 460

#include "../voxel_world.glsl"

layout(std430, set = 1, binding = 0) restrict buffer Params {
    vec4 camera_origin;
    vec4 camera_direction;
    vec4 hit_position;
    float near;
    float far;
    bool hit;
    float radius;    
} params;

layout(local_size_x = 1, local_size_y = 1, local_size_z = 1) in;
void main() {
    ivec3 grid_position;
    vec3 normal;
    int step_count = 0;
    float t;

    vec3 ray_origin = params.camera_origin.xyz;
    vec3 ray_dir = normalize(params.camera_direction.xyz);

    params.hit = voxelTraceWorld(ray_origin, ray_dir, vec2(params.near, params.far), t, grid_position, normal, step_count);
    if (params.hit) {
        params.hit_position = vec4(ray_origin + t * ray_dir, 1.0);
    }
}
