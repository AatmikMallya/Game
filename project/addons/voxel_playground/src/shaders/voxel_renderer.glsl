#[compute]
#version 460

#include "utility.glsl"
#include "voxel_world.glsl"

// ----------------------------------- STRUCTS -----------------------------------

struct Light {
    vec4 position;
    vec4 color;
};

// ----------------------------------- GENERAL STORAGE -----------------------------------

layout(set = 1, binding = 0, rgba8) restrict uniform writeonly image2D outputImage;
layout(set = 1, binding = 1, r32f) restrict uniform writeonly image2D depthBuffer;

layout(std430, set = 1, binding = 2) restrict buffer Params {
    vec4 background; //rgb, brightness
    int width;
    int height;
    float fov;
} params;

layout(std430, set = 1, binding = 3) restrict buffer Camera {
    mat4 view_projection;
    mat4 inv_view_projection;
    vec4 position;
    uint frame_index;
    float near;
    float far;
} camera;

// -------------------------- PROJECTILES (ray-marched spheres) --------------------------
layout(std430, set = 1, binding = 4) restrict buffer ProjectileParams {
    int projectile_count;
} projectileParams;

// Packed as vec4(x, y, z, radius) per projectile, up to MAX_PROJECTILES (defined at compile time)
#ifndef MAX_PROJECTILES
#define MAX_PROJECTILES 64
#endif
layout(std430, set = 1, binding = 5) restrict buffer ProjectileSpheres {
    vec4 projectile_spheres[MAX_PROJECTILES];
} projectileData;

// -------------------------- VOXEL ENTITIES --------------------------
#ifndef ENABLE_ENTITIES
#define ENABLE_ENTITIES 0
#endif
#ifndef MAX_ENTITIES
#define MAX_ENTITIES 32
#endif

struct EntityDescriptor {
    mat4 local_to_world;
    mat4 world_to_local;
    vec4 aabb_min;
    vec4 aabb_max;
    vec4 grid_size;
    vec4 brick_grid_size;
    float scale;
    uint brick_offset;
    uint voxel_offset;
    uint brick_count;
    uint enabled;
    float health;
    float _pad0; float _pad1; float _pad2;
};

#if ENABLE_ENTITIES
layout(std430, set = 1, binding = 6) restrict buffer EntityCount {
    int entity_count;
} entityCount;

layout(std430, set = 1, binding = 9) restrict buffer EntityDescriptors {
    EntityDescriptor entities[MAX_ENTITIES];
} entityDesc;

// Entity result sampler (written by entity_trace pass)
layout(set = 1, binding = 10) uniform sampler2D entityResultTex;
#endif // ENABLE_ENTITIES

#if ENABLE_ENTITIES
uint entityGetBrickIndex(ivec3 pos, int id) {
    ivec3 bgs = ivec3(entityDesc.entities[id].brick_grid_size.xyz);
    ivec3 brickCoord = pos / BRICK_EDGE_LENGTH;
    return uint(brickCoord.x + brickCoord.y * bgs.x + brickCoord.z * bgs.x * bgs.y);
}

uint entityGetVoxelIndexInBrick(ivec3 pos) {
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
    return uint(localPos.x + (localPos.y * BRICK_EDGE_LENGTH) + (localPos.z * BRICK_EDGE_LENGTH * BRICK_EDGE_LENGTH));
}

uint entityPosToIndex(ivec3 pos, int id) {
    if (!entityIsValidPos(pos, id)) return 0u;
    uint bidx = entityGetBrickIndex(pos, id);
    // fetch brick and compute index; brick.voxel_data_pointer is absolute in bricks in global entityVoxels
    Brick brick = entityBricks[entityDesc.entities[id].brick_offset + bidx];
    return brick.voxel_data_pointer * BRICK_VOLUME + entityGetVoxelIndexInBrick(pos);
}

Voxel entityGetVoxel(uint index) {
    return entityVoxels[index];
}

// ---------- Scalar entity trace (robust on Metal) ----------
bool entityTraceScalar(int id, vec3 ro_ws, vec3 rd_ws, out float t_ws, out vec3 normal_ws) {
    if (entityDesc.entities[id].enabled == 0u) return false;
    float eps = 1e-6;
    // Slab AABB in world space
    vec3 bmin = entityDesc.entities[id].aabb_min.xyz;
    vec3 bmax = entityDesc.entities[id].aabb_max.xyz;
    float rdx = (abs(rd_ws.x) > eps) ? rd_ws.x : ((rd_ws.x >= 0.0) ? eps : -eps);
    float rdy = (abs(rd_ws.y) > eps) ? rd_ws.y : ((rd_ws.y >= 0.0) ? eps : -eps);
    float rdz = (abs(rd_ws.z) > eps) ? rd_ws.z : ((rd_ws.z >= 0.0) ? eps : -eps);
    float tx1 = (bmin.x - ro_ws.x) / rdx; float tx2 = (bmax.x - ro_ws.x) / rdx; if (tx1 > tx2) { float t=tx1; tx1=tx2; tx2=t; }
    float ty1 = (bmin.y - ro_ws.y) / rdy; float ty2 = (bmax.y - ro_ws.y) / rdy; if (ty1 > ty2) { float t=ty1; ty1=ty2; ty2=t; }
    float tz1 = (bmin.z - ro_ws.z) / rdz; float tz2 = (bmax.z - ro_ws.z) / rdz; if (tz1 > tz2) { float t=tz1; tz1=tz2; tz2=t; }
    float tmin = max(max(tx1, ty1), tz1);
    float tmax = min(min(tx2, ty2), tz2);
    if (tmax < max(tmin, 0.0)) return false;

    // Transform to local
    vec3 ro = (entityDesc.entities[id].world_to_local * vec4(ro_ws, 1.0)).xyz;
    vec3 rd = normalize((entityDesc.entities[id].world_to_local * vec4(rd_ws, 0.0)).xyz);
    float scale = entityDesc.entities[id].scale;

    // Start at entry point in local
    vec3 p_entry_ws = ro_ws + rd_ws * max(tmin, 0.0);
    vec3 pL = (entityDesc.entities[id].world_to_local * vec4(p_entry_ws, 1.0)).xyz;

    int gx = int(floor(pL.x / scale));
    int gy = int(floor(pL.y / scale));
    int gz = int(floor(pL.z / scale));
    int sx = (rd.x > 0.0) ? 1 : -1;
    int sy = (rd.y > 0.0) ? 1 : -1;
    int sz = (rd.z > 0.0) ? 1 : -1;

    float invx = 1.0 / ((abs(rd.x) > eps) ? rd.x : ((rd.x >= 0.0) ? eps : -eps));
    float invy = 1.0 / ((abs(rd.y) > eps) ? rd.y : ((rd.y >= 0.0) ? eps : -eps));
    float invz = 1.0 / ((abs(rd.z) > eps) ? rd.z : ((rd.z >= 0.0) ? eps : -eps));
    float nextBx = float(gx + (sx > 0 ? 1 : 0)) * scale;
    float nextBy = float(gy + (sy > 0 ? 1 : 0)) * scale;
    float nextBz = float(gz + (sz > 0 ? 1 : 0)) * scale;
    float tMaxX = (nextBx - pL.x) * invx; if (tMaxX < 0.0) tMaxX = 0.0;
    float tMaxY = (nextBy - pL.y) * invy; if (tMaxY < 0.0) tMaxY = 0.0;
    float tMaxZ = (nextBz - pL.z) * invz; if (tMaxZ < 0.0) tMaxZ = 0.0;
    float tDeltaX = (scale) * abs(invx);
    float tDeltaY = (scale) * abs(invy);
    float tDeltaZ = (scale) * abs(invz);

    vec3 bmaxL = entityDesc.entities[id].brick_grid_size.xyz * scale * BRICK_EDGE_LENGTH; // not strictly needed
    float tEnd = tmax; // world-space end

    for (int it = 0; it < 512; ++it) {
        ivec3 gp = ivec3(gx, gy, gz);
        if (!entityIsValidPos(gp, id)) return false;
        uint bidx = entityGetBrickIndex(gp, id);
        Brick bk = entityBricks[entityDesc.entities[id].brick_offset + bidx];
        if (bk.occupancy_count > 0u) {
            uint vindex = bk.voxel_data_pointer * BRICK_VOLUME + entityGetVoxelIndexInBrick(gp);
            Voxel v = entityGetVoxel(vindex);
            if (!isVoxelAir(v)) {
                vec3 centerL = (vec3(gp) + vec3(0.5)) * scale;
                vec3 centerW = (entityDesc.entities[id].local_to_world * vec4(centerL, 1.0)).xyz;
                t_ws = dot(centerW - ro_ws, rd_ws);
                vec3 nL = vec3(0.0);
                if (tMaxX <= tMaxY && tMaxX <= tMaxZ) nL = vec3(-float(sx), 0.0, 0.0);
                else if (tMaxY <= tMaxX && tMaxY <= tMaxZ) nL = vec3(0.0, -float(sy), 0.0);
                else nL = vec3(0.0, 0.0, -float(sz));
                normal_ws = normalize((entityDesc.entities[id].local_to_world * vec4(nL, 0.0)).xyz);
                return true;
            }
        }
        if (tMaxX < tMaxY) {
            if (tMaxX < tMaxZ) { gx += sx; pL += rd * tMaxX; tMaxX += tDeltaX; }
            else { gz += sz; pL += rd * tMaxZ; tMaxZ += tDeltaZ; }
        } else {
            if (tMaxY < tMaxZ) { gy += sy; pL += rd * tMaxY; tMaxY += tDeltaY; }
            else { gz += sz; pL += rd * tMaxZ; tMaxZ += tDeltaZ; }
        }
        vec3 pW = (entityDesc.entities[id].local_to_world * vec4(pL, 1.0)).xyz;
        float tWorld = dot(pW - ro_ws, rd_ws);
        if (tWorld > tEnd) break;
    }
    return false;
}

bool entityTraceBrick(vec3 origin, vec3 direction, uint voxel_data_pointer, out uint voxelIndex, inout int step_count, inout vec3 normal, out ivec3 grid_position, out float t) {
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

    while (all(greaterThanEqual(grid_position, ivec3(0))) && all(lessThanEqual(grid_position, ivec3(7)))) {
        voxelIndex = voxel_data_pointer + uint(entityGetVoxelIndexInBrick(grid_position));
        if (!isVoxelAir(entityGetVoxel(voxelIndex))) return true;

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

bool entityTrace(int id, vec3 ro_ws, vec3 rd_ws, vec2 range, out Voxel voxel, out float t_ws, out ivec3 grid_position, out vec3 normal_ws, out int step_count) {
    if (entityDesc.entities[id].enabled == 0u) return false;
    step_count = 0; voxel = createAirVoxel();
    float epsilon = 1e-4;

    vec3 aabbMin = entityDesc.entities[id].aabb_min.xyz;
    vec3 aabbMax = entityDesc.entities[id].aabb_max.xyz;
    vec3 invDir = 1.0 / max(abs(rd_ws), vec3(epsilon)) * sign(rd_ws);
    vec3 t0 = (aabbMin - ro_ws) * invDir;
    vec3 t1 = (aabbMax - ro_ws) * invDir;
    vec3 tmin = min(t0, t1);
    vec3 tmax = max(t0, t1);
    float t_entry = max(max(tmin.x, tmin.y), tmin.z);
    float t_exit  = min(min(tmax.x, tmax.y), tmax.z);
    if (t_entry > t_exit || t_exit < 0.0) return false;

    // Transform ray to local space (meters)
    vec3 ro = (entityDesc.entities[id].world_to_local * vec4(ro_ws, 1.0)).xyz;
    vec3 rd = normalize((entityDesc.entities[id].world_to_local * vec4(rd_ws, 0.0)).xyz);

    float scale    = entityDesc.entities[id].scale;
    float brick_scale    = scale * BRICK_EDGE_LENGTH;

    vec3 bounds_min = vec3(0.0);
    vec3 bounds_max = vec3(entityDesc.entities[id].brick_grid_size.xyz) * brick_scale;

    vec3 invDirL = 1.0 / max(abs(rd), vec3(epsilon)) * sign(rd);
    vec3 t0L = (bounds_min - ro) * invDirL;
    vec3 t1L = (bounds_max - ro) * invDirL;
    vec3 tminL = min(t0L, t1L);
    vec3 tmaxL = max(t0L, t1L);
    float t_entryL = max(max(tminL.x, tminL.y), tminL.z);
    float t_exitL  = min(min(tmaxL.x, tmaxL.y), tmaxL.z);
    if (t_entryL > t_exitL || t_exitL < 0.0) return false;

    // initialize normal based on the entry point
    vec3 normal = vec3(0.0);
    if (t_entryL == tminL.x) normal = vec3(-sign(rd.x), 0.0, 0.0);
    else if (t_entryL == tminL.y) normal = vec3(0.0, -sign(rd.y), 0.0);
    else normal = vec3(0.0, 0.0, -sign(rd.z));

    float t = max(t_entryL, range.x);
    vec3 pos = ro + t * rd;
    pos = clamp(pos, bounds_min, bounds_max - vec3(epsilon));
    ivec3 brick_grid_position = ivec3(floor(pos / brick_scale));

    ivec3 step_dir   = ivec3(sign(rd));
    vec3 invAbsDir   = 1.0 / max(abs(rd), vec3(epsilon));
    vec3 factor      = step(vec3(0.0), rd);

    vec3 lowerDistance = (pos - vec3(brick_grid_position) * brick_scale);
    vec3 upperDistance = (((vec3(brick_grid_position) + vec3(1.0)) * brick_scale) - pos);
    vec3 tDelta      = brick_scale * invAbsDir;
    vec3 tMax        = vec3(t) + mix(lowerDistance, upperDistance, factor) * invAbsDir;

    while(step_count < MAX_RAY_STEPS && t < min(range.y, t_exitL)) {
        grid_position = brick_grid_position * BRICK_EDGE_LENGTH;
        if (!entityIsValidPos(grid_position, id)) break;

        uint bidx = entityGetBrickIndex(grid_position, id);
        Brick brick = entityBricks[entityDesc.entities[id].brick_offset + bidx];
        if (brick.occupancy_count > 0u) {
            vec3 lp = ((ro + t * rd) - vec3(grid_position) * scale) / (brick_scale) * BRICK_EDGE_LENGTH;
            uint voxelIndex;
            ivec3 local_brick_grid_position;
            float brick_t = 0.0;
            if (entityTraceBrick(lp, rd, (brick.voxel_data_pointer * BRICK_VOLUME), voxelIndex, step_count, normal, local_brick_grid_position, brick_t)) {
                t += brick_t * entityDesc.entities[id].scale;
                grid_position += local_brick_grid_position;
                voxel = entityGetVoxel(voxelIndex);
                // return t in world space
                vec3 hit_ls = ro + t * rd;
                vec3 hit_ws = (entityDesc.entities[id].local_to_world * vec4(hit_ls, 1.0)).xyz;
                t_ws = length(hit_ws - ro_ws);
                normal_ws = normalize((entityDesc.entities[id].local_to_world * vec4(normal, 0.0)).xyz);
                return true;
            }
        }

        float minT = min(min(tMax.x, tMax.y), tMax.z);
        vec3 mask = vec3(1) - step(vec3(epsilon), abs(tMax - vec3(minT)));
        vec3 ray_step = mask * step_dir;
        t = minT;
        tMax += mask * tDelta;        
        brick_grid_position += ivec3(ray_step);
        step_count++;
    }
    return false;
}
#endif // ENABLE_ENTITIES

// ----------------------------------- FUNCTIONS -----------------------------------

vec3 blinnPhongShading(vec3 baseColor, vec3 normal, vec3 lightDir, vec3 lightColor, vec3 viewDir, float shadow) {
    float NdotL = max(dot(normal, lightDir), 0.0);

    vec3 diffuse = NdotL * baseColor;

    vec3 specular = vec3(0.0);
    vec3 H = normalize(lightDir + viewDir);
    float NdotH = max(dot(normal, H), 0.0);
    specular = pow(NdotH, 10.0) * lightColor;

    vec3 ambient = baseColor;

    vec3 result = 0.25 * shadow * specular;
    result += (shadow * 0.5 + 0.5) * diffuse;
    result += 0.2 * ambient;
    return result;
}

layout(local_size_x = 32, local_size_y = 32, local_size_z = 1) in;
void main() {
    ivec2 pos = ivec2(gl_GlobalInvocationID.xy);
    if (pos.x >= params.width || pos.y >= params.height) return;

    vec2 screen_uv = vec2(pos + 0.5) / vec2(params.width, params.height);

    vec4 ndc = vec4(screen_uv * 2.0 - 1.0, 0.0, 1.0);
    ndc.y = -ndc.y;

#ifdef FORCE_DEBUG
    vec3 dbg = vec3(screen_uv, 0.0);
    imageStore(outputImage, pos, vec4(dbg, 1.0));
    imageStore(depthBuffer, pos, vec4(0.0, 0.0, 0.0, 0.0));
    return;
#endif

#if ENABLE_ENTITIES && defined(DEBUG_ENTITY_RESULT_READ)
    // Debug readback of entityResult: magenta if we have a finite t, gradient otherwise.
    vec4 erdbg = imageLoad(entityResult, pos);
    if (erdbg.r < 1e19) {
        imageStore(outputImage, pos, vec4(1.0, 0.0, 1.0, 1.0));
    } else {
        imageStore(outputImage, pos, vec4(vec3(screen_uv, 0.0), 1.0));
    }
    imageStore(depthBuffer, pos, vec4(0.0));
    return;
#endif

#if ENABLE_ENTITIES && defined(DEBUG_ENTITY_AABB_ONLY)
    // Scalar ray vs AABB test for entity 0 (no vector min/max/sign).
    vec3 ro = camera.position.xyz;
    vec4 world_pos4 = inverse(camera.view_projection) * ndc;
    world_pos4 /= world_pos4.w;
    vec3 rd = normalize(world_pos4.xyz - ro);

    vec3 bmin = entityDesc.entities[0].aabb_min.xyz;
    vec3 bmax = entityDesc.entities[0].aabb_max.xyz;

    float eps = 1e-6;
    float rdx = (abs(rd.x) > eps) ? rd.x : ((rd.x >= 0.0) ? eps : -eps);
    float rdy = (abs(rd.y) > eps) ? rd.y : ((rd.y >= 0.0) ? eps : -eps);
    float rdz = (abs(rd.z) > eps) ? rd.z : ((rd.z >= 0.0) ? eps : -eps);

    float tx1 = (bmin.x - ro.x) / rdx;
    float tx2 = (bmax.x - ro.x) / rdx;
    if (tx1 > tx2) { float tmp = tx1; tx1 = tx2; tx2 = tmp; }

    float ty1 = (bmin.y - ro.y) / rdy;
    float ty2 = (bmax.y - ro.y) / rdy;
    if (ty1 > ty2) { float tmp = ty1; ty1 = ty2; ty2 = tmp; }

    float tz1 = (bmin.z - ro.z) / rdz;
    float tz2 = (bmax.z - ro.z) / rdz;
    if (tz1 > tz2) { float tmp = tz1; tz1 = tz2; tz2 = tmp; }

    float tmin = max(max(tx1, ty1), tz1);
    float tmax = min(min(tx2, ty2), tz2);

    if (tmax >= max(tmin, 0.0)) {
        imageStore(outputImage, pos, vec4(1.0, 0.0, 1.0, 1.0));
    } else {
        imageStore(outputImage, pos, vec4(vec3(screen_uv, 0.0), 1.0));
    }
    imageStore(depthBuffer, pos, vec4(0.0));
    return;
#endif

#if ENABLE_ENTITIES && defined(DEBUG_ENTITY_READ_BRICK)
    // Probe entity bricks/voxels without any ray math.
    uint bbase = entityDesc.entities[0].brick_offset;
    Brick bb = entityBricks[bbase];
    uint vptr = bb.voxel_data_pointer; // in bricks
    uint vindex = vptr * BRICK_VOLUME; // first voxel of that brick
    Voxel vx = entityVoxels[vindex];
    // If non-air, draw magenta, else gradient.
    bool non_air = !isVoxelAir(vx);
    vec3 col = non_air ? vec3(1.0, 0.0, 1.0) : vec3(screen_uv, 0.0);
    imageStore(outputImage, pos, vec4(col, 1.0));
    imageStore(depthBuffer, pos, vec4(0.0));
    return;
#endif

    vec4 world_pos = inverse(camera.view_projection) * ndc;
    world_pos /= world_pos.w;
    vec3 ray_origin = camera.position.xyz;
    vec3 ray_dir = normalize(world_pos.xyz - ray_origin);
    ivec3 grid_position;
    vec3 normal;
    int step_count = 0;
    float t;
    vec3 color = vec3(0.0);

    Voxel voxel;
    
    // Entities: sample precomputed trace result (from entity_trace pass)
    float t_entity = 1e20; vec3 n_entity = vec3(0.0); int hit_entity_id = -1;
#if ENABLE_ENTITIES
    vec4 er = texelFetch(entityResultTex, pos, 0);
    t_entity = er.r;
    n_entity = er.gba;
    if (t_entity < 1e19) hit_entity_id = 0;
#endif

    // Check intersection with projectiles (spheres)
    float t_sphere = 1e20;
    vec3 n_sphere = vec3(0.0);
    bool hit_sphere = false;
    for (int i = 0; i < projectileParams.projectile_count && i < MAX_PROJECTILES; ++i) {
        vec3 c = projectileData.projectile_spheres[i].xyz;
        float r = projectileData.projectile_spheres[i].w;
        // Ray-sphere intersection (analytic)
        vec3 oc = ray_origin - c;
        float b = dot(oc, ray_dir);
        float c_term = dot(oc, oc) - r * r;
        float disc = b*b - c_term;
        if (disc >= 0.0) {
            float s = sqrt(disc);
            float t0 = -b - s;
            float t1 = -b + s;
            float t_candidate = (t0 > 0.0) ? t0 : ((t1 > 0.0) ? t1 : 1e20);
            if (t_candidate > camera.near && t_candidate < camera.far && t_candidate < t_sphere) {
                t_sphere = t_candidate;
                vec3 hp = ray_origin + t_candidate * ray_dir;
                n_sphere = normalize(hp - c);
                hit_sphere = true;
            }
        }
    }

    bool hit_world = voxelTraceWorld(ray_origin, ray_dir, vec2(camera.near, camera.far), voxel, t, grid_position, normal, step_count);

#ifdef DEBUG_WORLD_ONLY
    if (hit_world) {
        imageStore(outputImage, pos, vec4(0.0, 1.0, 0.0, 1.0)); // green = world hit
    } else {
        imageStore(outputImage, pos, vec4(0.0, 0.0, 1.0, 1.0)); // blue = sky/no world hit
    }
    imageStore(depthBuffer, pos, vec4(0.0));
    return;
#endif

    // Choose nearest between projectile sphere, entity, and world
    float t_nearest = 1e20; int mode = 0; // 1=proj,2=entity,3=world
    if (hit_sphere) { t_nearest = t_sphere; mode = 1; }
    if (hit_entity_id >= 0 && t_entity < t_nearest) { t_nearest = t_entity; mode = 2; }
    if (hit_world && t < t_nearest) { t_nearest = t; mode = 3; }

    if (mode == 1) {
        // Shade projectile as emissive fireball
        vec3 hitPos = ray_origin + t_sphere * ray_dir;
        vec3 baseColor = vec3(1.5, 0.5, 0.1); // bright orange emissive
        vec3 viewDir = normalize(camera.position.xyz - hitPos);
        float glow = 1.0;
        // Simple lighting for visibility
        color = baseColor * (1.0 + 0.5 * max(0.0, dot(n_sphere, normalize(voxelWorldProperties.sun_direction.xyz))));
        // Small rim highlight
        color += 0.2 * pow(1.0 - max(0.0, dot(n_sphere, -viewDir)), 2.0);
    } else if (mode == 2) {
#if ENABLE_ENTITIES
        // Shade entity hit as emissive slime
        vec3 hitPos = ray_origin + t_entity * ray_dir;
        vec3 baseColor = vec3(0.2, 0.9, 0.45);
        vec3 viewDir = normalize(camera.position.xyz - hitPos);
        color = baseColor * 1.4;
        color += 0.15 * pow(1.0 - max(0.0, dot(n_entity, -viewDir)), 2.0);
        #ifdef DEBUG_ENTITY_TINT
        color = vec3(1.0, 0.0, 1.0);
        #endif
#else
        // Entities disabled; ignore
        mode = 3;
#endif
    } else if (mode == 3) {
        vec3 hitPos = ray_origin + t * ray_dir;
        normal = normalize(normal);
        vec3 voxel_pos = vec3(grid_position) * voxelWorldProperties.scale;// + 0.5;
        vec3 baseColor = vec3(grid_position) / voxelWorldProperties.grid_size.xyz;
        float emission = getVoxelEmission(voxel);
        color = getVoxelColor(voxel, grid_position) * (1 + emission);
        if(isVoxelLiquid(voxel))
        {
            color += vec3(0.05 * sin(0.0167 * voxelWorldProperties.frame + 0.2 * (grid_position.x + grid_position.y + grid_position.z)));
            color += vec3(((voxel.data & 0xFu) > 0) ? 0.5 : 0);
        }

        vec3 voxel_view_dir = normalize(camera.position.xyz - voxel_pos);

        // direct illumination
        if(emission < 1) {
            float shadow = computeShadow(hitPos, normal, voxelWorldProperties.sun_direction.xyz);
            float ao = computeAmbientOcclusion(hitPos, grid_position, normal) * 0.7 + 0.3;
            color = ao * blinnPhongShading(color, normal, normalize(voxelWorldProperties.sun_direction.xyz), voxelWorldProperties.sun_color.rgb, voxel_view_dir, shadow);
        }
    } else {
        color = sampleSkyColor(ray_dir);
    }
    // depth = camera.far / (camera.far - camera.near) * (1.0 - camera.near / depth);
    //visualize steps
    // if(step_count < 1000)
    //     color = vec3(step_count * 0.001);
    // else
    //     color = vec3(1,0,0);


    float depth = 0.0f;
    imageStore(outputImage, pos, vec4(color, 1.0));
    imageStore(depthBuffer, pos, vec4(depth, 0.0, 0.0, 0.0));
}
#if ENABLE_ENTITIES
// Entity trace result (t, normal.xyz) written by entity_trace.glsl
layout(set = 1, binding = 10, rgba32f) uniform readonly image2D entityResult;
#endif
