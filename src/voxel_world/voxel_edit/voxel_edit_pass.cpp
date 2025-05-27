

#include "voxel_edit_pass.h"
#include <godot_cpp/core/print_string.hpp>  // For print_line()

using namespace godot;


VoxelEditPass::VoxelEditPass(String shader_path, RenderingDevice * rd, RID voxel_bricks, RID voxel_data, RID voxel_properties, const Vector3i size) : _size(size){

    voxel_edit_pass_params edit_properties = {
        Vector4(0, 0, 0, 1), // camera_origin
        Vector4(0, 0, -1, 0), // camera_direction
        Vector4(0, 0, 0, 1), // hit_position
        0.1f, // near
        100.0f, // range
        false, // hit
        _radius // radius
    };

    ray_cast_shader = new ComputeShader("res://addons/voxel_playground/src/shaders/voxel_edit/raycast.glsl", rd);
    add_voxel_buffers(ray_cast_shader, voxel_bricks, voxel_data, voxel_properties);
    ray_cast_shader->finish_create_uniforms();

    edit_shader = new ComputeShader(shader_path, rd);
    add_voxel_buffers(edit_shader, voxel_bricks, voxel_data, voxel_properties);
    edit_shader->finish_create_uniforms();
}

void VoxelEditPass::update(float delta)
{
    if (ray_cast_shader == nullptr || !ray_cast_shader->check_ready() || edit_shader == nullptr || !edit_shader->check_ready()) 
    {
        UtilityFunctions::printerr("VoxelEditPass::update() edit shader is null or not ready");
        return;
    }

    const Vector3 group_size = Vector3(8, 8, 8);
    const Vector3i group_count = Vector3i(std::ceil(_radius / group_size.x), std::ceil(_radius / group_size.y), std::ceil(_radius / group_size.z));
    edit_shader->compute(group_count, false);
}
