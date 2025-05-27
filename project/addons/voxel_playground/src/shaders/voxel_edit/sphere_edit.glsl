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

layout(local_size_x = 8, local_size_y = 8, local_size_z = 8) in;
void main() {
    ivec3 pos = ivec3(gl_GlobalInvocationID.xyz);
    ivec3 world_pos = ivec3(params.hit_position.xyz) + pos - ivec3(params.radius);
    if (!isValidPos(pos)) return;

    // Calculate the distance from the center of the sphere
    vec3 center = params.hit_position.xyz;
    float d = length(vec3(world_pos) - center);

    // Set the voxel data based on the distance
    if (d < params.radius) { // Inside the sphere
        uint brick_index = getBrickIndex(world_pos);
        uint voxel_index = voxelBricks[brick_index].voxel_data_pointer * BRICK_VOLUME + getVoxelIndexInBrick(world_pos);     

        atomicAdd(voxelBricks[brick_index].occupancy_count, 1);
        voxelData[voxel_index].data = 1; 
    }
}
