#ifndef VOXEL_WORLD_GLSL
#define VOXEL_WORLD_GLSL



#define MAX_RAY_STEPS 1000
#define BRICK_EDGE_LENGTH 8
#define BRICK_VOLUME 512 

struct Brick { // we may add color to this for simple LOD
    uint occupancy_count;      // mask for voxels in the brick; 0 means the brick is empty
    uint voxel_data_pointer;  // index of the first voxel in the brick (voxels stored in Morton order)
};

struct Voxel {
    uint data;
};

layout(std430, set = 0, binding = 0) buffer VoxelWorldProperties {
    ivec4 grid_size;
    ivec4 brick_grid_size;
    vec4 sky_color;
    vec4 ground_color;
    vec4 sun_color;
    vec4 sun_direction;
    float scale;
    int frame;
} voxelWorldProperties;

layout(std430, set = 0, binding = 1) buffer VoxelWorldBricks {
    Brick voxelBricks[];
};

layout(std430, set = 0, binding = 2) buffer VoxelWorldData {
    Voxel voxelData[];
};

layout(std430, set = 0, binding = 3) buffer VoxelWorldData2 {
    Voxel voxelData2[];
};



// -------------------------------------- VOXEL DATA --------------------------------------

const uint VOXEL_TYPE_AIR = 0;
const uint VOXEL_TYPE_SOLID = 1;
const uint VOXEL_TYPE_WATER = 2;
const uint VOXEL_TYPE_LAVA = 3;
const vec3 DEFAULT_WATER_COLOR = vec3(0.1, 0.3, 0.8);
const vec3 DEFAULT_LAVA_COLOR = vec3(4.0, 0.6, 0.1);

Voxel createVoxel(uint type, vec3 color) {
    Voxel voxel;
    voxel.data = (type & 0xFF) << 24; // Store type in the highest byte
    voxel.data |= (compress_color16(color) & 0xFFFF) << 8; //store color in the next 2 bytes
    return voxel;
}

Voxel createAirVoxel() {
    return createVoxel(VOXEL_TYPE_AIR, vec3(0.0));
}

Voxel createWaterVoxel() {
    Voxel voxel = createVoxel(VOXEL_TYPE_WATER, DEFAULT_WATER_COLOR);
    // voxel.data |= 127;
    return voxel;
}

Voxel createLavaVoxel() {
    Voxel voxel = createVoxel(VOXEL_TYPE_LAVA, DEFAULT_LAVA_COLOR);
    // voxel.data |= 127;
    return voxel;
}

Voxel createGrassVoxel(ivec3 pos) {
   vec3 color = randomizedColor(vec3(.2, .9, .45), pos);    
    return createVoxel(VOXEL_TYPE_SOLID, color);
}


Voxel createRockVoxel(ivec3 pos) {
    vec3 color = randomizedColor(vec3(.24, .25, .32), pos);
    return createVoxel(VOXEL_TYPE_SOLID, color);
}

bool isVoxelType(Voxel voxel, uint type) {
    return ((voxel.data >> 24) & 0xFF) == (type & 0xFF);
}

bool equalsVoxelType(Voxel a, Voxel b) {
    return ((a.data >> 24) & 0xFF) == ((b.data >> 24) & 0xFF);
}

bool isVoxelAir(Voxel voxel) {
    return isVoxelType(voxel, VOXEL_TYPE_AIR);
}

bool isVoxelLiquid(Voxel voxel) {
    return isVoxelType(voxel, VOXEL_TYPE_WATER) || isVoxelType(voxel, VOXEL_TYPE_LAVA);
}

bool isVoxelSolid(Voxel voxel) {
    return !isVoxelType(voxel, VOXEL_TYPE_AIR) && !isVoxelLiquid(voxel);
}

bool isVoxelDynamic(Voxel voxel) {
    return isVoxelLiquid(voxel);
}

vec3 getVoxelColor(Voxel voxel) {
    uint color = (voxel.data >> 8) & 0xFFFF;
    return decompress_color16(color);
}

//0 is base value
float getVoxelEmission(Voxel voxel) {
     return isVoxelType(voxel, VOXEL_TYPE_LAVA) ? 1 : 0;
}

Voxel getPreviousVoxel(uint index)
{
    return voxelWorldProperties.frame % 2 == 0 ? voxelData2[index] : voxelData[index];
}

Voxel getVoxel(uint index)
{
    return voxelWorldProperties.frame % 2 == 0 ? voxelData[index] : voxelData2[index];
}

void setVoxel(uint index, Voxel voxel) {
    if (voxelWorldProperties.frame % 2 == 0)
        voxelData[index] = voxel;
    else 
        voxelData2[index] = voxel;    
}

void setPreviousVoxel(uint index, Voxel voxel) {
    if (voxelWorldProperties.frame % 2 == 0)
        voxelData2[index] = voxel;
    else 
        voxelData[index] = voxel;    
}


void setBothVoxelBuffers(uint index, Voxel voxel)
{
    voxelData[index] = voxel;
    voxelData2[index] = voxel;
}

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


#define USE_MORTON_ORDER
uint getVoxelIndexInBrick(ivec3 pos) {
    ivec3 localPos = pos % BRICK_EDGE_LENGTH;
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

// ivec3 stepMask(vec3 sideDist) {
//     // Yoinked from https://www.shadertoy.com/view/l33XWf
//     bvec3 move;
//     bvec3 pon=lessThan(sideDist.xyz,sideDist.yzx);
//     move.x=pon.x && !pon.z;
//     move.y=pon.y && !pon.x;
//     move.z=!(move.x||move.y);
//     return ivec3(move);
// }

// -------------------------------------- RAYCASTING --------------------------------------
bool voxelTraceBrick(vec3 origin, vec3 direction, uint voxel_data_pointer, out uint voxelIndex, inout int step_count, inout vec3 normal, out ivec3 grid_position, out float t) {
    origin = clamp(origin, vec3(0.001), vec3(7.999));
    grid_position = ivec3(floor(origin));

    ivec3 step_dir   = ivec3(sign(direction));
    vec3 invAbsDir   = 1.0 / max(abs(direction), vec3(1e-4));
    vec3 factor      = step(vec3(0.0), direction);

    t = 0.0;

    vec3 lowerDistance = (origin - vec3(grid_position));
    vec3 upperDistance = (((vec3(grid_position) + vec3(1.0))) - origin);
    vec3 tDelta      = invAbsDir;
    vec3 tMax        = vec3(t) + mix(lowerDistance, upperDistance, factor) * invAbsDir;

    while (all(greaterThanEqual(grid_position, ivec3(0))) &&
           all(lessThanEqual(grid_position, ivec3(7)))) {
        voxelIndex = voxel_data_pointer + uint(getVoxelIndexInBrick(grid_position));
        if (!isVoxelAir(getVoxel(voxelIndex))) 
            return true;

        float minT = min(min(tMax.x, tMax.y), tMax.z);
        vec3 mask = vec3(1) - step(vec3(1e-4), abs(tMax - vec3(minT)));
        vec3 ray_step = mask * step_dir;

        t = minT;
        tMax += mask * tDelta;        
        grid_position += ivec3(ray_step);
        step_count++;
        normal = -ray_step;
    }
    
    return false;
}

bool voxelTraceWorld(vec3 origin, vec3 direction, vec2 range, out Voxel voxel, out float t, out ivec3 grid_position, out vec3 normal, out int step_count) {
    step_count = 0;
    grid_position = ivec3(0);
    float scale    = voxelWorldProperties.scale * BRICK_EDGE_LENGTH;

    voxel = createAirVoxel();

    //ensure ray is in bounds
    vec3 bounds_min = vec3(0.0);
    vec3 bounds_max = vec3(voxelWorldProperties.brick_grid_size.xyz) * scale;

    vec3 invDir = 1.0 / (direction + 1e-9);
    vec3 t0 = (bounds_min - origin) * invDir;
    vec3 t1 = (bounds_max - origin) * invDir;

    vec3 tmin = min(t0, t1);
    vec3 tmax = max(t0, t1);
    float t_entry = max(max(tmin.x, tmin.y), tmin.z);
    float t_exit  = min(min(tmax.x, tmax.y), tmax.z);
    if (t_entry > t_exit || t_exit < 0.0)
        return false;

    t = max(t_entry, range.x);
    vec3 pos = origin + t * direction;
    float epsilon = 1e-4;
    pos = clamp(pos, bounds_min, bounds_max - vec3(epsilon));
    grid_position = ivec3(floor(pos / scale));


    //DDA
    ivec3 step_dir   = ivec3(sign(direction));
    vec3 invAbsDir   = 1.0 / max(abs(direction), vec3(1e-9));
    vec3 factor      = step(vec3(0.0), direction);

    vec3 lowerDistance = (pos - vec3(grid_position) * scale);
    vec3 upperDistance = (((vec3(grid_position) + vec3(1.0)) * scale) - pos);
    vec3 tDelta      = scale * invAbsDir;
    vec3 tMax        = vec3(t) + mix(lowerDistance, upperDistance, factor) * invAbsDir;

    normal = vec3(0,1,0);

    while(step_count < MAX_RAY_STEPS && t < min(range.y, t_exit)) {
        if (!isValidPos(grid_position * BRICK_EDGE_LENGTH))
            break;
        
        uint brick_index = getBrickIndex(grid_position * BRICK_EDGE_LENGTH);
        Brick brick = voxelBricks[brick_index];
        if (brick.occupancy_count > 0) {
            uint voxelIndex;
            pos = ((origin + t * direction) - grid_position * scale) * BRICK_EDGE_LENGTH;
            ivec3 brick_grid_position;
            float brick_t = 0.0;
            if (voxelTraceBrick(pos, direction, brick.voxel_data_pointer * BRICK_VOLUME, voxelIndex, step_count, normal, brick_grid_position, brick_t)) {
                t += brick_t * scale / BRICK_EDGE_LENGTH;
                grid_position = grid_position * BRICK_EDGE_LENGTH + brick_grid_position;
                voxel = getVoxel(voxelIndex);
                return true;
            }
        }

        float minT = min(min(tMax.x, tMax.y), tMax.z);
        vec3 mask = vec3(1) - step(vec3(epsilon), abs(tMax - vec3(minT)));
        vec3 ray_step = mask * step_dir;

        t = minT;
        tMax += mask * tDelta;        
        grid_position += ivec3(ray_step);
        normal = -ray_step;
        step_count++;        
    }
    
    return false;
}

// -------------------------------------- Rendering --------------------------------------
vec3 sampleSkyColor(vec3 direction) {
    float intensity = max(0.0, 0.5 + dot(direction, vec3(0.0, 0.5, 0.0)));
    vec3 sky = mix(voxelWorldProperties.ground_color.rgb, voxelWorldProperties.sky_color.rgb, intensity);

    float sun_intensity = pow(max(0.0, dot(direction, voxelWorldProperties.sun_direction.xyz)), 50.0);

    return mix(sky, voxelWorldProperties.sun_color.rgb, sun_intensity);
}

float computeShadow(vec3 position, vec3 normal, vec3 lightDir) {
    float t; ivec3 grid_position; vec3 normal_out; int step_count; Voxel voxel;
    return voxelTraceWorld(position + normal * 0.001, lightDir, vec2(0.0, 100.0), voxel, t, grid_position, normal_out, step_count) ? 0.6 : 1.0;
}

#endif // VOXEL_WORLD_GLSL
