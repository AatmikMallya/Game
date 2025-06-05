#[compute]
#version 460

#include "../utility.glsl"
#include "../voxel_world.glsl"

vec3 randomizedColor(vec3 base_color, ivec3 pos) {
    uvec3 hash_value = hash(pos);
    vec2 rn = pcg2d(hash_value.xy);
    vec3 color = rgb2hsv(base_color);
    color.x += rn.x * 0.025;
    color.yz *= 0.9 + rn.y * 0.2;
    return saturate(hsv2rgb(color));
}

Voxel createGrassVoxel(ivec3 pos) {
   vec3 color = randomizedColor(vec3(.2, .9, .45), pos);    
    return createVoxel(VOXEL_TYPE_SOLID, color);
}


Voxel createRockVoxel(ivec3 pos) {
    vec3 color = randomizedColor(vec3(.24, .25, .32), pos);
    return createVoxel(VOXEL_TYPE_SOLID, color);
}

float distanceToBoundary(vec3 pos, vec3 halfBoxSize) {
    vec3 d = abs(pos - halfBoxSize) - halfBoxSize;
    float dist = length(max(d, 0.0)) + min(max(d.x, max(d.y, d.z)), 0.0);

    return saturate(0.01 * (1 - dist));
}

layout(local_size_x = 8, local_size_y = 8, local_size_z = 8) in;
void main() {
    ivec3 pos = ivec3(gl_GlobalInvocationID.xyz);
    if (!isValidPos(pos)) return;

    uint brick_index = getBrickIndex(pos);
    uint voxel_index = voxelBricks[brick_index].voxel_data_pointer * BRICK_VOLUME + getVoxelIndexInBrick(pos);     

    // Calculate the distance from the center of the sphere
    vec3 world_pos = pos * voxelWorldProperties.scale;

    // Set the voxel data based on the distance
    float noise = fbm(world_pos * 0.1);
    float dist_to_boundary = distanceToBoundary(pos, voxelWorldProperties.grid_size.xyz * 0.5); 
    if (pos.y < 121 && 0.4 < noise * dist_to_boundary) { //  
        atomicAdd(voxelBricks[brick_index].occupancy_count, 1);
        if(pos.y > 64 + noise * 64)
            voxelData[voxel_index] = createGrassVoxel(pos);
        else 
            voxelData[voxel_index] = createRockVoxel(pos);
        // voxelData[voxel_index] =createVoxel(VOXEL_TYPE_SOLID, vec3(dist_to_boundary));
    } else if(pos.y > 200) {
        // Create a sky voxel at the top of the world
        voxelData[voxel_index] = createWaterVoxel();
        atomicAdd(voxelBricks[brick_index].occupancy_count, 1);
    }
    else { // Outside the sphere
        voxelData[voxel_index] = createAirVoxel();
    }
}