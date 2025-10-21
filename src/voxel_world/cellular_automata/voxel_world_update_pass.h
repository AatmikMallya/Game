
#ifndef VOXEL_WORLD_UPDATE_PASS_H
#define VOXEL_WORLD_UPDATE_PASS_H

#include <godot_cpp/classes/node3d.hpp>
#include <godot_cpp/classes/rendering_device.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/rid.hpp>
#include <godot_cpp/classes/time.hpp>

#include "gdcs/include/gdcs.h"
#include "voxel_world/voxel_properties.h"

using namespace godot;

class VoxelWorldUpdatePass
{

  public:
    VoxelWorldUpdatePass(String shader_path, RenderingDevice *rd, VoxelWorldRIDs& voxel_world_rids, const Vector3i size);
    ~VoxelWorldUpdatePass() {};

    void update(float delta);

    // Performance profiling getters (microseconds for CPU, milliseconds for GPU)
    uint64_t get_time_liquid_us() const { return _time_liquid_us; }
    uint64_t get_time_freeze_us() const { return _time_freeze_us; }
    uint64_t get_time_cleanup_us() const { return _time_cleanup_us; }

    // GPU timing getters (milliseconds)
    float get_gpu_time_liquid_ms() const { return _gpu_time_liquid_ms; }
    float get_gpu_time_freeze_ms() const { return _gpu_time_freeze_ms; }
    float get_gpu_time_cleanup_ms() const { return _gpu_time_cleanup_ms; }

  private:
    ComputeShader *automata_cs_1 = nullptr;
    ComputeShader *automata_cs_2 = nullptr;
    ComputeShader *cleanup_shader = nullptr;
    Vector3i _size;

    // Performance profiling (CPU: microseconds, GPU: milliseconds)
    uint64_t _time_liquid_us = 0;
    uint64_t _time_freeze_us = 0;
    uint64_t _time_cleanup_us = 0;

    float _gpu_time_liquid_ms = 0.0f;
    float _gpu_time_freeze_ms = 0.0f;
    float _gpu_time_cleanup_ms = 0.0f;
};

#endif // VOXEL_WORLD_UPDATE_PASS_H
