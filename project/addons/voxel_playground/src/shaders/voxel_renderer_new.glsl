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

// -------------------------- ENTITIES (voxel-based) --------------------------
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
    float _pad0, _pad1, _pad2;
};

layout(std430, set = 1, binding = 6) restrict buffer EntityCount {
    int entity_count;
} entityCount;

layout(std430, set = 1, binding = 7) restrict buffer EntityBricksBuf {
    Brick entityBricks[];
};

layout(std430, set = 1, binding = 8) restrict buffer EntityVoxelsBuf {
    Voxel entityVoxels[];
};

layout(std430, set = 1, binding = 9) restrict buffer EntityDescriptors {
    EntityDescriptor entities[MAX_ENTITIES];
} entityDesc;

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

    // Check entity AABBs and raymarch voxels
    float t_entity = 1e20;
    vec3 entity_color = vec3(0);
    vec3 n_entity = vec3(0);
    bool hit_entity = false;
    bool hit_any_aabb = false;  // DEBUG

    for (int eid = 0; eid < entityCount.entity_count && eid < MAX_ENTITIES; ++eid) {
        EntityDescriptor ent = entityDesc.entities[eid];

        if (ent.enabled == 0u) continue;

        // Extract entity world position from local_to_world matrix (column 3)
        vec3 entity_world_pos = vec3(ent.local_to_world[3][0], ent.local_to_world[3][1], ent.local_to_world[3][2]);

        // World-space AABB (no rotation support yet)
        vec3 aabb_min = entity_world_pos + ent.aabb_min.xyz;
        vec3 aabb_max = entity_world_pos + ent.aabb_max.xyz;

        // AABB ray intersection
        const float eps = 1e-7;
        vec3 safe_dir = ray_dir;
        safe_dir.x = abs(safe_dir.x) < eps ? (safe_dir.x >= 0.0 ? eps : -eps) : safe_dir.x;
        safe_dir.y = abs(safe_dir.y) < eps ? (safe_dir.y >= 0.0 ? eps : -eps) : safe_dir.y;
        safe_dir.z = abs(safe_dir.z) < eps ? (safe_dir.z >= 0.0 ? eps : -eps) : safe_dir.z;

        vec3 inv_dir = 1.0 / safe_dir;
        vec3 t0 = (aabb_min - ray_origin) * inv_dir;
        vec3 t1 = (aabb_max - ray_origin) * inv_dir;
        vec3 tmin_vec = min(t0, t1);
        vec3 tmax_vec = max(t0, t1);
        float tmin = max(max(tmin_vec.x, tmin_vec.y), tmin_vec.z);
        float tmax = min(min(tmax_vec.x, tmax_vec.y), tmax_vec.z);

        // Check for valid intersection
        if (tmin <= tmax && tmax > 0.0) {
            // Hit! Render colored box
            entity_color = vec3(0.0, 1.0, 0.0);  // Green
            t_entity = max(tmin, camera.near);
            hit_entity = true;
            n_entity = vec3(0, 1, 0);  // Simple up normal
            break;
        }
    }

    bool hit_world = voxelTraceWorld(ray_origin, ray_dir, vec2(camera.near, camera.far), voxel, t, grid_position, normal, step_count);

    // Proper depth-sorted compositing
    // Entity is closest if: entity hit AND (no sphere hit OR entity closer than sphere) AND (no world hit OR entity closer than world)
    bool entity_closest = hit_entity && (!hit_sphere || t_entity < t_sphere) && (!hit_world || (hit_world && t_entity < t));
    bool sphere_closest = !entity_closest && hit_sphere && (!hit_world || (hit_world && t_sphere < t));

    if (entity_closest) {
        color = entity_color;
    } else if (sphere_closest) {
        // Shade projectile as emissive fireball
        vec3 hitPos = ray_origin + t_sphere * ray_dir;
        vec3 baseColor = vec3(1.5, 0.5, 0.1); // bright orange emissive
        vec3 viewDir = normalize(camera.position.xyz - hitPos);
        float glow = 1.0;
        // Simple lighting for visibility
        color = baseColor * (1.0 + 0.5 * max(0.0, dot(n_sphere, normalize(voxelWorldProperties.sun_direction.xyz))));
        // Small rim highlight
        color += 0.2 * pow(1.0 - max(0.0, dot(n_sphere, -viewDir)), 2.0);
    } else if (hit_world) {
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
