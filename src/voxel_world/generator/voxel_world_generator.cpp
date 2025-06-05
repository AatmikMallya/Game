
#include "voxel_world_generator.h"
#include <godot_cpp/core/print_string.hpp>  // For print_line()

using namespace godot;


void VoxelWorldGenerator::initialize_brick_grid(RenderingDevice* rd, RID voxel_bricks, RID voxel_data, RID properties, const Vector3i brick_grid_size) {
    compute_shader = new ComputeShader("res://addons/voxel_playground/src/shaders/generators/initialize_brick_grid.glsl", rd);

    compute_shader->add_existing_buffer(voxel_bricks, RenderingDevice::UNIFORM_TYPE_STORAGE_BUFFER, 0, 0);
    compute_shader->add_existing_buffer(voxel_data, RenderingDevice::UNIFORM_TYPE_STORAGE_BUFFER, 1, 0);
    compute_shader->add_existing_buffer(properties, RenderingDevice::UNIFORM_TYPE_STORAGE_BUFFER, 2, 0);
    compute_shader->finish_create_uniforms();
    
    const Vector3 group_size = Vector3(8, 8, 8);
    const Vector3i group_count = Vector3i(std::ceil(brick_grid_size.x / group_size.x), std::ceil(brick_grid_size.y / group_size.y), std::ceil(brick_grid_size.z / group_size.z));
    compute_shader->compute(group_count, false);
}

void VoxelWorldGenerator::populate(RenderingDevice* rd, RID voxel_bricks, RID voxel_data, RID properties, const Vector3i size)
{
    compute_shader = new ComputeShader("res://addons/voxel_playground/src/shaders/generators/floating_island.glsl", rd);

    compute_shader->add_existing_buffer(voxel_bricks, RenderingDevice::UNIFORM_TYPE_STORAGE_BUFFER, 0, 0);
    compute_shader->add_existing_buffer(voxel_data, RenderingDevice::UNIFORM_TYPE_STORAGE_BUFFER, 1, 0);
    compute_shader->add_existing_buffer(properties, RenderingDevice::UNIFORM_TYPE_STORAGE_BUFFER, 2, 0);
    compute_shader->finish_create_uniforms();
    
    const Vector3 group_size = Vector3(8, 8, 8);
    const Vector3i group_count = Vector3i(std::ceil(size.x / group_size.x), std::ceil(size.y / group_size.y), std::ceil(size.z / group_size.z));
    compute_shader->compute(group_count, false);

    UtilityFunctions::print("VoxelWorldGenerator::populate() finished");
}
