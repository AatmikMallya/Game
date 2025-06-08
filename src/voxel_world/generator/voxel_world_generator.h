#ifndef VOXEL_WORLD_GENERATOR_H
#define VOXEL_WORLD_GENERATOR_H

#include <godot_cpp/classes/node3d.hpp>
#include <godot_cpp/classes/rendering_device.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/rid.hpp>

#include "gdcs/include/gdcs.h"
#include "voxel_world/voxel_properties.h"

using namespace godot;

class VoxelWorldGenerator
{

    ComputeShader *compute_shader = nullptr;

  public:
    VoxelWorldGenerator() {};
    ~VoxelWorldGenerator() {};

    void initialize_brick_grid(RenderingDevice *rd, VoxelWorldRIDs voxel_world_rids, const Vector3i brick_grid_size);

    void populate(RenderingDevice *rd, VoxelWorldRIDs voxel_world_rids, const Vector3i size);
};

#endif // VOXEL_WORLD_GENERATOR_H
