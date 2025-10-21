

#include "voxel_world_update_pass.h"
#include <godot_cpp/core/print_string.hpp>  // For print_line()

using namespace godot;


VoxelWorldUpdatePass::VoxelWorldUpdatePass(String shader_path, RenderingDevice * rd, VoxelWorldRIDs& voxel_world_rids, const Vector3i size) : _size(size){
    automata_cs_1 = new ComputeShader(shader_path, rd);
    voxel_world_rids.add_voxel_buffers(automata_cs_1);
    automata_cs_1->finish_create_uniforms();

    automata_cs_2 = new ComputeShader("res://addons/voxel_playground/src/shaders/automata/freeze_lava.glsl", rd);
    voxel_world_rids.add_voxel_buffers(automata_cs_2);
    automata_cs_2->finish_create_uniforms();

    cleanup_shader = new ComputeShader("res://addons/voxel_playground/src/shaders/automata/cleanup_pass.glsl", rd);
    voxel_world_rids.add_voxel_buffers(cleanup_shader);
    cleanup_shader->finish_create_uniforms();
}

void VoxelWorldUpdatePass::update(float delta)
{
    if (automata_cs_1 == nullptr || cleanup_shader == nullptr)
    {
        UtilityFunctions::printerr("VoxelWorldUpdatePass::update() compute shader is null");
        return;
    }

    { // Liquid automata pass
        uint64_t start = Time::get_singleton()->get_ticks_usec();
        const Vector3 group_size = Vector3(8, 8, 8);
        const Vector3i group_count = Vector3i(std::ceil(_size.x / group_size.x), std::ceil(_size.y / group_size.y), std::ceil(_size.z / group_size.z));
        automata_cs_1->compute(group_count, true);  // Enable sync for GPU timing
        uint64_t end = Time::get_singleton()->get_ticks_usec();
        _time_liquid_us = end - start;
        _gpu_time_liquid_ms = automata_cs_1->get_last_gpu_time_ms();
    }

    { // Freeze lava pass
        uint64_t start = Time::get_singleton()->get_ticks_usec();
        const Vector3 group_size = Vector3(8, 8, 8);
        const Vector3i group_count = Vector3i(std::ceil(_size.x / group_size.x), std::ceil(_size.y / group_size.y), std::ceil(_size.z / group_size.z));
        automata_cs_2->compute(group_count, true);  // Enable sync for GPU timing
        uint64_t end = Time::get_singleton()->get_ticks_usec();
        _time_freeze_us = end - start;
        _gpu_time_freeze_ms = automata_cs_2->get_last_gpu_time_ms();
    }

    { // Cleanup pass
        uint64_t start = Time::get_singleton()->get_ticks_usec();
        const Vector3 thread_span = Vector3(2, 4, 2);
        const Vector3 group_size = Vector3(4, 2, 4);
        const Vector3 brick_span = thread_span * group_size;
        const Vector3i group_count = Vector3i(std::ceil(_size.x / brick_span.x), std::ceil(_size.y / brick_span.y), std::ceil(_size.z / brick_span.z));
        cleanup_shader->compute(group_count, true);  // Enable sync for GPU timing
        uint64_t end = Time::get_singleton()->get_ticks_usec();
        _time_cleanup_us = end - start;
        _gpu_time_cleanup_ms = cleanup_shader->get_last_gpu_time_ms();
    }

}
