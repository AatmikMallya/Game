#[compute]
#version 460

#include "voxel_world.glsl"

layout(local_size_x = 8, local_size_y = 8, local_size_z = 8) in;

// A simple pseudo-random generator based on voxel position.
// It’s not cryptographically secure but is sufficient for shuffling directions.
uint pseudoRandom(ivec3 pos) {
    uint x = uint(pos.x) * 73856093u;
    uint y = uint(pos.y) * 19349663u;
    uint z = uint(pos.z) * 83492791u;
    return x ^ y ^ z;
}

void main() {
    ivec3 pos = ivec3(gl_GlobalInvocationID.xyz);
    if (!isValidPos(pos)) return;
    
    uint brick_index = getBrickIndex(pos);
    if(voxelBricks[brick_index].occupancy_count == 0) 
        return;

    uint voxel_index = voxelBricks[brick_index].voxel_data_pointer * BRICK_VOLUME + getVoxelIndexInBrick(pos); 
    
    // Only process voxels that contain at least one unit of water (or liquid)
    if (voxelData[voxel_index].data >= 1) {
        // First try to move straight down.
        ivec3 downPos = pos - ivec3(0, 1, 0);
        if (isValidPos(downPos)) {
            uint down_brick_index = getBrickIndex(downPos);
            uint downIndex = voxelBricks[down_brick_index].voxel_data_pointer * BRICK_VOLUME + getVoxelIndexInBrick(downPos); 
            if (voxelData[downIndex].data <= 1) {
                // Transfer one unit downward.
                atomicAdd(voxelData[downIndex].data, 1);
                atomicAdd(voxelBricks[down_brick_index].occupancy_count, 1);

                atomicAdd(voxelData[voxel_index].data, -1);
                atomicAdd(voxelBricks[brick_index].occupancy_count, -1);
                return;
            }
        }
        
        // If the voxel cannot fall, try moving laterally.
        ivec3 directions[4] = ivec3[](
            ivec3(1, 0, 0),  // right
            ivec3(-1, 0, 0), // left
            ivec3(0, 0, 1),  // forward
            ivec3(0, 0, -1)  // backward
        );
        
        // Shuffle the directions array to randomize the order.
        uint randVal = pseudoRandom(pos);
        for (int i = 0; i < 4; i++) {
            int j = int(randVal % uint(4 - i)) + i;
            ivec3 temp = directions[i];
            directions[i] = directions[j];
            directions[j] = temp;
            randVal /= 4u;  // Update randVal for further shuffles
        }
        
        // Try each lateral direction.
        for (int i = 0; i < 4; i++) {
            ivec3 sidePos = pos + directions[i];
            if (isValidPos(sidePos)) {
                uint side_brick_index = getBrickIndex(sidePos);
                uint sideIndex = voxelBricks[side_brick_index].voxel_data_pointer * BRICK_VOLUME + getVoxelIndexInBrick(sidePos); 
                if (voxelData[sideIndex].data <= 1) {
                    // Transfer one unit of water to the side.
                    atomicAdd(voxelData[sideIndex].data, 1);
                    atomicAdd(voxelBricks[side_brick_index].occupancy_count, 1);

                    atomicAdd(voxelData[voxel_index].data, -1);
                    atomicAdd(voxelBricks[brick_index].occupancy_count, -1);
                    return;
                }
            }
        }
    }
}
