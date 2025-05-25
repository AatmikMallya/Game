
#ifndef VOXEL_WORLD_UPDATE_PASS_H
#define VOXEL_WORLD_UPDATE_PASS_H

#include <godot_cpp/classes/node3d.hpp>
#include <godot_cpp/classes/rendering_device.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/rid.hpp>

#include "gdcs/include/gdcs.h"
#include "voxel_world/properties.h"

using namespace godot;

class VoxelWorldUpdatePass
{

  public:
    VoxelWorldUpdatePass(String shader_path, RenderingDevice *rd, RID voxel_bricks, RID voxel_data, RID properties, const Vector3i size);
    ~VoxelWorldUpdatePass() {};

    void update(float delta);

  private:
    ComputeShader *compute_shader = nullptr;
    Vector3i _size;
};

#endif // VOXEL_WORLD_UPDATE_PASS_H
