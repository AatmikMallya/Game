#ifndef PATH_TRACING_CAMERA_H
#define PATH_TRACING_CAMERA_H

#include "gdcs/include/gdcs.h"
#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/classes/image.hpp>
#include <godot_cpp/classes/texture2drd.hpp>
#include <godot_cpp/classes/node3d.hpp>
#include <godot_cpp/classes/rd_texture_format.hpp>
#include <godot_cpp/classes/rd_texture_view.hpp>
#include <godot_cpp/classes/texture_rect.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/packed_byte_array.hpp>
#include <godot_cpp/variant/projection.hpp>
#include <godot_cpp/variant/transform3d.hpp>
#include <godot_cpp/variant/utility_functions.hpp>
#include <godot_cpp/classes/display_server.hpp>
#include <godot_cpp/classes/time.hpp>
#include <voxel_world.h>

using namespace godot;

class VoxelCamera : public Node3D
{
    GDCLASS(VoxelCamera, Node3D);

  public:

    static constexpr int MAX_PROJECTILES = 64;
    static constexpr int MAX_ENTITIES = 32;

    struct RenderParameters // match the struct on the gpu
    {
        Vector4 backgroundColor;
        int width;
        int height;
        float fov;

        PackedByteArray to_packed_byte_array()
        {
            PackedByteArray byte_array;
            byte_array.resize(sizeof(RenderParameters));
            std::memcpy(byte_array.ptrw(), this, sizeof(RenderParameters));
            return byte_array;
        }
    };

    struct CameraParameters // match the struct on the gpu
    {
        float vp[16];
        float ivp[16];
        Vector4 cameraPosition;
        int frame_index;
        float nearPlane;
        float farPlane;

        PackedByteArray to_packed_byte_array()
        {
            PackedByteArray byte_array;
            byte_array.resize(sizeof(CameraParameters));
            std::memcpy(byte_array.ptrw(), this, sizeof(CameraParameters));
            return byte_array;
        }
    };

    struct ProjectileParameters // gpu struct: current projectile count
    {
        int projectile_count;

        PackedByteArray to_packed_byte_array()
        {
            PackedByteArray byte_array;
            byte_array.resize(sizeof(ProjectileParameters));
            std::memcpy(byte_array.ptrw(), this, sizeof(ProjectileParameters));
            return byte_array;
        }
    };

  protected:
    static void _bind_methods();

  public:
    void _notification(int what);

    float get_fov() const;
    void set_fov(float value);

    TextureRect *get_output_texture() const;
    void set_output_texture(TextureRect *value);

    VoxelWorld *get_voxel_world() const;
    void set_voxel_world(VoxelWorld* value);

    // Performance profiling getters (returns milliseconds)
    float get_time_camera_update() const { return _time_camera_update_us / 1000.0f; }
    float get_time_raymarching() const { return _time_raymarching_us / 1000.0f; }
    float get_time_total_render() const { return _time_total_render_us / 1000.0f; }

    // GPU timing getter (returns milliseconds)
    float get_gpu_time_raymarching() const { return _gpu_time_raymarching_ms; }

    // ---------------- Projectile (ray-marched) API ----------------
    // Register a sphere projectile to be ray-marched. Returns an id to update/remove later.
    int register_projectile(const Vector3 &position, float radius);
    void update_projectile(int id, const Vector3 &position, float radius);
    void remove_projectile(int id);
    // ---------------- Entities (voxel) API ----------------
    void discover_entities();

  private:
    void init();
    void update(double delta);
    void clear_compute_shader();
    void render();

    float fov = 90.0f;
    // int num_bounces = 4;

    ComputeShader *cs = nullptr;
    ComputeShader *cs_entities = nullptr;
    TextureRect *output_texture_rect = nullptr;
    VoxelWorld *voxel_world = nullptr;
    Ref<Image> output_image;
    Ref<Image> depth_image;
    Ref<Image> entity_result_image;
    Ref<Texture2DRD> output_texture;

    RenderParameters render_parameters;
    CameraParameters camera_parameters;
    Projection projection_matrix;

    // BUFFER IDs
    RID output_texture_rid;
    RID depth_texture_rid;
    RID render_parameters_rid;
    RID camera_parameters_rid;
    RID entity_result_rid;
    RID projectile_parameters_rid;
    RID projectile_spheres_rid;
    RID entity_count_rid;
    RID entity_descriptors_rid;
    RID entity_bricks_rid;
    RID entity_voxels_rid;
    RenderingDevice *_rd;

    // CPU-side projectile cache. Packed as vec4(x,y,z,radius) per element.
    // We keep a free-list of ids for reuse.
    struct ProjectileEntry { Vector4 data; bool active; };
    Vector<ProjectileEntry> _projectiles; // fixed capacity MAX_PROJECTILES
    ProjectileParameters _projectile_params;

    // Map debug spheres for voxel entities (store projectile ids)
    Vector<int> _entity_debug_spheres;

    // ---------------- Entities CPU cache ----------------
    struct EntityDescriptorCPU {
        float local_to_world[16];
        float world_to_local[16];
        float aabb_min[4];
        float aabb_max[4];
        float grid_size[4];        // xyz voxels
        float brick_grid_size[4];  // xyz bricks
        float scale;
        uint32_t brick_offset;
        uint32_t voxel_offset;
        uint32_t brick_count;
        uint32_t enabled;
        float health;
        float pad[6];
    };
    static_assert(sizeof(EntityDescriptorCPU) % 16 == 0, "EntityDescriptorCPU size must be multiple of 16 for std430 arrays");
    Vector<EntityDescriptorCPU> _entities; // CPU copy of descriptors (size <= MAX_ENTITIES)

    // Aggregated arrays
    std::vector<Brick> _all_entity_bricks;
    std::vector<Voxel> _all_entity_voxels;

    // Performance profiling (microseconds)
    uint64_t _time_camera_update_us = 0;
    uint64_t _time_raymarching_us = 0;
    uint64_t _time_total_render_us = 0;

    // GPU timing (milliseconds)
    float _gpu_time_raymarching_ms = 0.0f;
};

#endif // PATH_TRACING_CAMERA_H
