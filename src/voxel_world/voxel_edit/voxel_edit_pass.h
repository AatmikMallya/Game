
#ifndef VOXEL_WORLD_UPDATE_PASS_H
#define VOXEL_WORLD_UPDATE_PASS_H

#include <godot_cpp/classes/node3d.hpp>
#include <godot_cpp/classes/rendering_device.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/rid.hpp>

#include "gdcs/include/gdcs.h"
#include "voxel_world/properties.h"

using namespace godot;

class VoxelEditPass
{
  public:
    VoxelEditPass(String edit_shader_path, RenderingDevice *rd, RID voxel_bricks, RID voxel_data, RID properties, const Vector3i size);
    ~VoxelEditPass() {};

    void update(float delta);

  private:
    ComputeShader *ray_cast_shader = nullptr;
    ComputeShader *edit_shader = nullptr;
    Vector3i _size;
    float _radius = 0.5f;

  struct voxel_edit_pass_params
  {
    Vector4 camera_origin;
    Vector4 camera_direction;
    Vector4 hit_position;
    float near;
    float far;
    bool hit;
    float radius;    
  };
  
};

#endif // VOXEL_WORLD_UPDATE_PASS_H
