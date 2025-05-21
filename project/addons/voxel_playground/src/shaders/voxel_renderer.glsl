#[compute]
#version 460

#include "voxel_world.glsl"

#define MAX_RAY_STEPS 10000

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


// ----------------------------------- VOXEL RAYTRACING FUNCTION -----------------------------------
float voxelRaytrace(vec3 origin, vec3 direction, out ivec3 grid_position) {
    grid_position = ivec3(0);

    int gridWidth  = voxelWorldProperties.width;
    int gridHeight = voxelWorldProperties.height;
    int gridDepth  = voxelWorldProperties.depth;
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

    float t = max(t_entry, camera.near);
    vec3 pos = origin + t * direction;
    
    float epsilon = 0.001;
    pos = clamp(pos, bounds_min, bounds_max - vec3(epsilon));
    
    grid_position = ivec3(floor(pos / scale));

    ivec3 step_dir = ivec3(sign(direction));
    
    vec3 invAbsDir = 1.0 /  max(abs(direction), vec3(1e-9));
    vec3 factor = step(vec3(0.0), direction);

    vec3 lowerDistance = (pos - vec3(grid_position) * scale);
    vec3 upperDistance = (((vec3(grid_position) + vec3(1.0)) * scale) - pos);

    vec3 tMax = vec3(t) + mix(lowerDistance, upperDistance, factor) * invAbsDir;

    vec3 tDelta = scale * abs(invDir);    
    float maxDistance = min(camera.far, t_exit);
    
    // Main loop: branchless selection for stepping.
    for (int i = 0; i < MAX_RAY_STEPS; i++) {
        if (!isValidPos(grid_position))
            break;
        
        if (voxelWorldData.voxelData[posToIndex(grid_position)].data != 0)
            return t;
        
        // Create a one-hot mask choosing the minimum of tMax.x, tMax.y, tMax.z:
        vec3 mask = step(tMax.xyz, tMax.yzx) * step(tMax.xyz, tMax.zxy);
        t = mask.x * tMax.x + mask.y * tMax.y + mask.z * tMax.z;
        tMax += mask * tDelta;
        grid_position += ivec3(
            int(mask.x * step_dir.x),
            int(mask.y * step_dir.y),
            int(mask.z * step_dir.z)
        );
        
        if(t > maxDistance)
            break;
    }
    
    return -1.0;
}


float computeAmbientOcclusion(vec3 hitPos) {
    // Convert hitPos into voxel grid space.
    // Assuming each voxel occupies a unit cell in grid space.
    float scale = voxelWorldProperties.scale;    
    vec3 scaledPos = hitPos / scale + 0.5;

    ivec3 gridCoord = ivec3(floor(scaledPos));    
    vec3 localFrac = fract(abs(scaledPos));

    float occ = 0.0;
    float totalWeight = 0.0;

    // Loop over the eight corners of the cell.
    for (int dz = 0; dz < 2; dz++) {
        for (int dy = 0; dy < 2; dy++) {
            for (int dx = 0; dx < 2; dx++) {
                ivec3 neighbor = gridCoord + ivec3(dx, dy, dz);
                
                // Compute the per-axis weights.
                // float weightX = (dx == 0) ? (1.0 - localFrac.x) : localFrac.x;
                // float weightY = (dy == 0) ? (1.0 - localFrac.y) : localFrac.y;
                // float weightZ = (dz == 0) ? (1.0 - localFrac.z) : localFrac.z;
                // float weight = weightX * weightY * weightZ;
                vec3 w = fract(localFrac - vec3(neighbor));
                float weight = w.x * w.y * w.z;
                
                // Only sample valid neighbors.
                if (isValidPos(neighbor)) {
                    int index = posToIndex(neighbor);
                    // Assume a nonzero voxel means it’s occluding.
                    if (voxelWorldData.voxelData[index].data != 0) {
                        occ += weight;
                    }
                }
                totalWeight += weight;
            }
        }
    }
    
    // Compute the ambient occlusion.
    // If many neighbors are solid, occ will be high and we want a dark (low AO) result.
    float ao = 1.0 - (occ / totalWeight);
    
    // Optionally, adjust the bias with an exponent.
    return ao;//pow(ao, 0.1);
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
    float t = voxelRaytrace(ray_origin, ray_dir, grid_position);
    vec3 color = vec3(0.0);
    if (t >= 0.0) {
        vec3 hitPos = ray_origin + t * ray_dir;
        vec3 baseColor = vec3(grid_position) / vec3(voxelWorldProperties.width, voxelWorldProperties.height, voxelWorldProperties.depth);
        color= baseColor;
        
        float ao = computeAmbientOcclusion(hitPos);
        
        // color = vec3(ao);
        // color = vec3((t - camera.near) / (camera.far -camera.near ) );
    } else {
        color = sampleSky(ray_dir);
    }


    // vec3 color = sampleMiddleSlice(screen_uv);

    // depth = camera.far / (camera.far - camera.near) * (1.0 - camera.near / depth);
    float depth = 0.0f;
    imageStore(outputImage, pos, vec4(color, 1.0));
    imageStore(depthBuffer, pos, vec4(depth, 0.0, 0.0, 0.0));
}