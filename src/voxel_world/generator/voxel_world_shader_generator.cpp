#include "voxel_world_shader_generator.h"

using namespace godot;

void VoxelWorldShaderGenerator::generate(RenderingDevice* rd, VoxelWorldRIDs& voxel_world_rids, const VoxelWorldProperties& properties)
{
    ComputeShader compute_shader = ComputeShader("res://addons/voxel_playground/src/shaders/generators/floating_island.glsl", rd);
    voxel_world_rids.add_voxel_buffers(&compute_shader);
    compute_shader.finish_create_uniforms();

    const Vector3 group_size = Vector3(8, 8, 8);
    const auto size = properties.grid_size;
    const Vector3i group_count = Vector3i(std::ceil(size.x / group_size.x), std::ceil(size.y / group_size.y), std::ceil(size.z / group_size.z));
    compute_shader.compute(group_count, false);
}
