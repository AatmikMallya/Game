

#include "voxel_world_update_pass.h"
#include <godot_cpp/core/print_string.hpp>  // For print_line()

using namespace godot;


VoxelWorldUpdatePass::VoxelWorldUpdatePass(String shader_path, RenderingDevice * rd, RID voxel_bricks, RID voxel_data, RID properties, const Vector3i size) : _size(size){
    compute_shader = new ComputeShader(shader_path, rd);

    compute_shader->add_existing_buffer(voxel_bricks, RenderingDevice::UNIFORM_TYPE_STORAGE_BUFFER, 0, 0);
    compute_shader->add_existing_buffer(voxel_data, RenderingDevice::UNIFORM_TYPE_STORAGE_BUFFER, 1, 0);
    compute_shader->add_existing_buffer(properties, RenderingDevice::UNIFORM_TYPE_STORAGE_BUFFER, 2, 0);
    compute_shader->finish_create_uniforms();
}

void VoxelWorldUpdatePass::update(float delta)
{
    if (compute_shader == nullptr)
    {
        UtilityFunctions::printerr("VoxelWorldUpdatePass::update() compute shader is null");
        return;
    }
    const Vector3 group_size = Vector3(8, 8, 8);
    const Vector3i group_count = Vector3i(std::ceil(_size.x / group_size.x), std::ceil(_size.y / group_size.y), std::ceil(_size.z / group_size.z));
    compute_shader->compute(group_count, false);
}
