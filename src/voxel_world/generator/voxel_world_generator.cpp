
#include "voxel_world_generator.h"

using namespace godot;

void VoxelWorldGenerator::initialize_brick_grid(RenderingDevice* rd, VoxelWorldRIDs& voxel_world_rids, const VoxelWorldProperties &properties) {
    ComputeShader compute_shader = ComputeShader("res://addons/voxel_playground/src/shaders/generators/initialize_brick_grid.glsl", rd);
    voxel_world_rids.add_voxel_buffers(&compute_shader);
    compute_shader.finish_create_uniforms();

    const Vector3 group_size = Vector3(8, 8, 8);
    const auto brick_grid_size = properties.brick_grid_size;
    const Vector3i group_count = Vector3i(std::ceil(brick_grid_size.x / group_size.x), std::ceil(brick_grid_size.y / group_size.y), std::ceil(brick_grid_size.z / group_size.z));
    compute_shader.compute(group_count, false);
}
