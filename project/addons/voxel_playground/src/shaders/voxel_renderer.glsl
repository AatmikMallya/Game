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


// ----------------------------------- FUNCTIONS -----------------------------------

vec3 sampleMiddleSlice(vec2 screen_uv) {
    int x = int(floor(screen_uv.x * float(voxelWorldProperties.grid_size.x)));
    int y = int(floor(screen_uv.y * float(voxelWorldProperties.grid_size.y)));
    int z = voxelWorldProperties.grid_size.z / 4;
    
    x = clamp(x, 0, voxelWorldProperties.grid_size.x - 1);
    y = clamp(y, 0, voxelWorldProperties.grid_size.y - 1);
    z = clamp(z, 0, voxelWorldProperties.grid_size.z - 1);
    
    uint index = posToIndex(ivec3(x, y, z));

    uint voxelValue = voxelData[index].data;    
    float intensity = float(voxelValue);    
    return vec3(intensity);
}

vec3 blinnPhongShading(vec3 baseColor, vec3 normal, vec3 lightDir, vec3 lightColor, vec3 viewDir) {
    // return baseColor;
    vec3 diffuse = max(dot(normal, lightDir), 0.0) * baseColor;
    vec3 H = normalize(lightDir + viewDir);
    float NdotH = max(dot(normal, H), 0);

    vec3 ambient = baseColor * sampleSkyColor(reflect(viewDir, normal));
    vec3 specular = pow(NdotH, 10.0) * lightColor;
    // vec3 specular = 0.5 * pow(max(dot(reflect(-lightDir, normal), viewDir), 0.0), 100.0) * lightColor;
    return 0.25 * specular + 1.0 * diffuse + 0.2 * ambient;
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


    if (voxelTraceWorld(ray_origin, ray_dir, vec2(camera.near, camera.far), voxel, t, grid_position, normal, step_count)) {
        vec3 hitPos = ray_origin + t * ray_dir;
        normal = normalize(normal);
        vec3 voxel_pos = vec3(grid_position) * voxelWorldProperties.scale;// + 0.5;
        vec3 baseColor = vec3(grid_position) / voxelWorldProperties.grid_size.xyz;
        // color = hsv2rgb(rgb2hsv(baseColor));
        // voxel.data = ~0;
        color = getVoxelColor(voxel);


        color *= 0.05 * dot(normal, vec3(0.25, 0.35, 0.4)) + 0.95; //discolor different faces slightly.

        vec3 voxel_view_dir = normalize(camera.position.xyz - voxel_pos);

        // direct illumination
        float shadow = computeShadow(hitPos, normal, voxelWorldProperties.sun_direction.xyz);
        // color = blinnPhongShading(color, normal, voxelWorldProperties.sun_direction.xyz, voxelWorldProperties.sun_color.rgb, voxel_view_dir);
        color *= shadow;

    } else {
        color = sampleSkyColor(ray_dir);
    }

    // color = sampleMiddleSlice(screen_uv);

    // depth = camera.far / (camera.far - camera.near) * (1.0 - camera.near / depth);
    float depth = 0.0f;
    imageStore(outputImage, pos, vec4(color, 1.0));
    imageStore(depthBuffer, pos, vec4(depth, 0.0, 0.0, 0.0));
}