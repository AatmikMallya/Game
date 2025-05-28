#ifndef VOXEL_WORLD_COLLIDER_H
#define VOXEL_WORLD_COLLIDER_H

#include <godot_cpp/classes/node3d.hpp>
#include <godot_cpp/classes/rendering_device.hpp>
#include <godot_cpp/variant/rid.hpp>
#include <godot_cpp/core/class_db.hpp>

#include "voxel_world/voxel_properties.h"
#include "gdcs/include/gdcs.h"

using namespace godot;

class VoxelWorldCollider {

    ComputeShader *fetch_data_shader = nullptr;

public:
    VoxelWorldCollider(RenderingDevice *rd, RID voxel_bricks, RID voxel_data, RID properties,
                               const Vector3i brick_grid_size) {};
    ~VoxelWorldCollider() {
    };

    void generate(const Vector3 position, const Vector3 size);
};

#endif // VOXEL_WORLD_COLLIDER_H
