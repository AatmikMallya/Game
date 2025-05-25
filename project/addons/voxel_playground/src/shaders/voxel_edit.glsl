#[compute]
#version 460

#include "voxel_world.glsl"

layout(std430, set = 1, binding = 2) restrict buffer Params {
    float radius;
    float
} params;

layout(local_size_x = 8, local_size_y = 8, local_size_z = 8) in;
void main() {
    ivec3 pos = ivec3(gl_GlobalInvocationID.xyz);
    if (!isValidPos(pos)) return;
    
    int index = posToIndex(pos);

}
