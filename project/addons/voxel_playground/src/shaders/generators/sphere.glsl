#[compute]
#version 460

#include "../voxel_world.glsl"

layout(local_size_x = 8, local_size_y = 8, local_size_z = 8) in;
void main() {
    ivec3 pos = ivec3(gl_GlobalInvocationID.xyz);
    if (pos.x >= voxelWorldProperties.width || pos.y >= voxelWorldProperties.height || pos.z >= voxelWorldProperties.depth) return;

    // Calculate the distance from the center of the sphere
    vec3 center = vec3(voxelWorldProperties.width * 0.5f, voxelWorldProperties.height * 0.5f, voxelWorldProperties.depth * 0.5f);
    float radius = min(voxelWorldProperties.width, min(voxelWorldProperties.height, voxelWorldProperties.depth)) * 0.5f;
    float distance = length(vec3(pos) - center);
    // Set the voxel data based on the distance
    int index = posToIndex(pos);
    if (distance < radius) {
        voxelWorldData.voxelData[index].data = 255; // Inside the sphere
    } else {
        voxelWorldData.voxelData[index].data = 0; // Outside the sphere
    }
}