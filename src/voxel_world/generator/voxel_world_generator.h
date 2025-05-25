#ifndef VOXEL_WORLD_GENERATOR_H
#define VOXEL_WORLD_GENERATOR_H

#include <godot_cpp/classes/node3d.hpp>
#include <godot_cpp/classes/rendering_device.hpp>
#include <godot_cpp/variant/rid.hpp>
#include <godot_cpp/core/class_db.hpp>

#include "voxel_world/properties.h"
#include "gdcs/include/gdcs.h"

using namespace godot;

class VoxelWorldGenerator {

    ComputeShader *compute_shader = nullptr;

public:
    VoxelWorldGenerator() {};
    ~VoxelWorldGenerator() {
    };

    void initialize_brick_grid(RenderingDevice *rd, RID voxel_bricks, RID voxel_data, RID properties,
                               const Vector3i brick_grid_size);

    void populate(RenderingDevice *rd, RID voxel_bricks, RID voxel_data, RID properties, const Vector3i size);
};

#endif // VOXEL_WORLD_GENERATOR_H
