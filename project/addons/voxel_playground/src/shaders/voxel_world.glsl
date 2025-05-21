#ifndef VOXEL_WORLD_GLSL
#define VOXEL_WORLD_GLSL

struct Voxel {
    int data;
};

layout(std430, set = 0, binding = 0) buffer VoxelWorldData {
    Voxel voxelData[]; // Adjust size as needed
} voxelWorldData;

layout(std430, set = 0, binding = 1) buffer VoxelWorldProperties {
    int width;
    int height;
    int depth;
    float scale;
} voxelWorldProperties;

// -------------------------------------- UTILS --------------------------------------
bool isValidPos(ivec3 pos) {
    return pos.x >= 0 && pos.x < voxelWorldProperties.width &&
           pos.y >= 0 && pos.y < voxelWorldProperties.height &&
           pos.z >= 0 && pos.z < voxelWorldProperties.depth;
}

int posToIndex(ivec3 pos) {
    return pos.x + pos.y * voxelWorldProperties.width + pos.z * voxelWorldProperties.width * voxelWorldProperties.height;
}

ivec3 worldToGrid(vec3 pos) {
    return ivec3(pos / voxelWorldProperties.scale);
}

Voxel getVoxel(vec3 pos) {
    ivec3 gridPos = worldToGrid(pos);
    if (isValidPos(gridPos)) {
        int index = posToIndex(gridPos);
        return voxelWorldData.voxelData[index];
    }
    return Voxel(0);
}

ivec3 indexToPos(int index) {
    int area = voxelWorldProperties.width * voxelWorldProperties.height;       
    int z = index / area;            
    int remainder = index - z * area;
    int y = remainder / voxelWorldProperties.width;       
    int x = remainder % voxelWorldProperties.width;       
    return ivec3(x, y, z);
}

#endif // VOXEL_WORLD_GLSL