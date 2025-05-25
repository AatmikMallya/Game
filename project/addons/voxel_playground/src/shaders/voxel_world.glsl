#ifndef VOXEL_WORLD_GLSL
#define VOXEL_WORLD_GLSL

#define MAX_RAY_STEPS 10000
#define BRICK_EDGE_LENGTH 8
#define BRICK_VOLUME 512 

struct Brick { // we may add color to this for simple LOD
    int occupancy_count;      // amount of voxels in the brick; 0 means the brick is empty
    uint voxel_data_pointer;  // index of the first voxel in the brick (voxels stored in Morton order)
};

struct Voxel {
    uint data;
};

layout(std430, set = 0, binding = 0) buffer VoxelWorldBricks {
    Brick voxelBricks[];
};

layout(std430, set = 0, binding = 1) buffer VoxelWorldData {
    Voxel voxelData[];
};

layout(std430, set = 0, binding = 2) buffer VoxelWorldProperties {
    ivec4 grid_size;

    ivec4 brick_grid_size;
    float scale;
} voxelWorldProperties;

// -------------------------------------- UTILS --------------------------------------
bool isValidPos(ivec3 pos) {
    return pos.x >= 0 && pos.x < voxelWorldProperties.grid_size.x &&
           pos.y >= 0 && pos.y < voxelWorldProperties.grid_size.y &&
           pos.z >= 0 && pos.z < voxelWorldProperties.grid_size.z;
}

uint getBrickIndex(ivec3 pos) {
    ivec3 brickCoord = pos / BRICK_EDGE_LENGTH;
    return brickCoord.x + brickCoord.y * voxelWorldProperties.brick_grid_size.x + brickCoord.z * voxelWorldProperties.brick_grid_size.x * voxelWorldProperties.brick_grid_size.y;
}

uint getVoxelIndexInBrick(ivec3 pos) {
    ivec3 localPos = pos % BRICK_EDGE_LENGTH;
    return uint(localPos.x +
           (localPos.y * BRICK_EDGE_LENGTH) +
           (localPos.z * BRICK_EDGE_LENGTH * BRICK_EDGE_LENGTH));
}

uint posToIndex(ivec3 pos) {
    if (!isValidPos(pos)) return 0;
    return voxelBricks[getBrickIndex(pos)].voxel_data_pointer * BRICK_VOLUME + getVoxelIndexInBrick(pos);
}

ivec3 worldToGrid(vec3 pos) {
    return ivec3(pos / voxelWorldProperties.scale);
}

// -------------------------------------- RAYCASTING --------------------------------------
float voxelRaytrace(vec3 origin, vec3 direction, vec2 range, out ivec3 grid_position, out vec3 normal, out int step_count) {
    step_count = 0;
    grid_position = ivec3(0);
    normal = vec3(0, 1, 0);

    int gridWidth  = voxelWorldProperties.grid_size.x;
    int gridHeight = voxelWorldProperties.grid_size.y;
    int gridDepth  = voxelWorldProperties.grid_size.z;
    float scale    = voxelWorldProperties.scale;

    vec3 bounds_min = vec3(0.0);
    vec3 bounds_max = vec3(float(gridWidth), float(gridHeight), float(gridDepth)) * scale;

    vec3 invDir = 1.0 / (direction + 1e-9);
    vec3 t0 = (bounds_min - origin) * invDir;
    vec3 t1 = (bounds_max - origin) * invDir;
    vec3 tmin = min(t0, t1);
    vec3 tmax = max(t0, t1);
    float t_entry = max(max(tmin.x, tmin.y), tmin.z);
    float t_exit  = min(min(tmax.x, tmax.y), tmax.z);
    if (t_entry > t_exit || t_exit < 0.0)
        return -1.0;

    float t = max(t_entry, range.x);
    vec3 pos = origin + t * direction;
    float epsilon = 1e-4;
    pos = clamp(pos, bounds_min, bounds_max - vec3(epsilon));
    grid_position = ivec3(floor(pos / scale));

    ivec3 step_dir   = ivec3(sign(direction));
    vec3 invAbsDir   = 1.0 / max(abs(direction), vec3(1e-9));
    vec3 factor      = step(vec3(0.0), direction);

    vec3 lowerDistance = (pos - vec3(grid_position) * scale);
    vec3 upperDistance = (((vec3(grid_position) + vec3(1.0)) * scale) - pos);
    vec3 tDelta      = scale * abs(invDir);
    vec3 tMax        = vec3(t) + mix(lowerDistance, upperDistance, factor) * invAbsDir;

    while(step_count < MAX_RAY_STEPS) {
        if (!isValidPos(grid_position))
            break;
        
        uint brick_index = getBrickIndex(grid_position);
        Brick brick = voxelBricks[brick_index];
        if (brick.occupancy_count == 0) {
            ivec3 brick_min = (grid_position / BRICK_EDGE_LENGTH) * BRICK_EDGE_LENGTH;
            ivec3 brick_max = brick_min + ivec3(BRICK_EDGE_LENGTH);
            vec3 brickMinWorld = vec3(brick_min) * scale;
            vec3 brickMaxWorld = vec3(brick_max) * scale;
            vec3 t0_b = (brickMinWorld - pos) * invDir;
            vec3 t1_b = (brickMaxWorld - pos) * invDir;
            vec3 tmin_b = min(t0_b, t1_b);
            vec3 tmax_b = max(t0_b, t1_b);
            float t_exit_brick = min(min(tmax_b.x, tmax_b.y), tmax_b.z);            
            t += t_exit_brick + epsilon;
            pos = origin + t * direction;
            grid_position = ivec3(floor(pos / scale));            
            lowerDistance = (pos - vec3(grid_position) * scale);
            upperDistance = (((vec3(grid_position) + vec3(1.0)) * scale) - pos);
            tMax = vec3(t) + mix(lowerDistance, upperDistance, factor) * invAbsDir;
            continue;
        }

        uint voxelIndex = brick.voxel_data_pointer * BRICK_VOLUME + uint(getVoxelIndexInBrick(grid_position));
        if (voxelData[voxelIndex].data != 0)
            return t;

        float minT = min(min(tMax.x, tMax.y), tMax.z);
        vec3 mask = vec3(1) - step(vec3(epsilon), abs(tMax - vec3(minT)));
        vec3 ray_step = mask * step_dir;

        t = minT;
        tMax += mask * tDelta;        
        grid_position += ivec3(ray_step);
        normal = -ray_step;
        
        if (t > min(range.y, t_exit))
            break;

        step_count++;
    }
    
    return -1.0;
}

#endif // VOXEL_WORLD_GLSL
