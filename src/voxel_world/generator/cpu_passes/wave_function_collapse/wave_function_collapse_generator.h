#ifndef WAVE_FUNCTION_COLLAPSE_GENERATOR_H
#define WAVE_FUNCTION_COLLAPSE_GENERATOR_H

#include "voxel_world/generator/voxel_world_generator.h"
#include <godot_cpp/classes/node3d.hpp>
#include <godot_cpp/classes/rendering_device.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/rid.hpp>

#include "../../voxel_world_generator_cpu_pass.h"
#include "voxel_world/data/voxel_data_vox.h"

using namespace godot;

class WaveFunctionCollapseGenerator : public VoxelWorldGeneratorCPUPass {
    GDCLASS(WaveFunctionCollapseGenerator, VoxelWorldGeneratorCPUPass)

public:
    void set_target_grid_size(const Vector3i &size) { target_grid_size = size; }
    Vector3i get_target_grid_size() const { return target_grid_size; }

    void set_voxel_scale(float scale) { voxel_scale = scale; }
    float get_voxel_scale() const { return voxel_scale; }

    virtual bool generate(std::vector<Voxel> &result_voxels, const Vector3i bounds_min, const Vector3i bounds_max, const VoxelWorldProperties &properties) = 0;

    static void _bind_methods() {
        ClassDB::bind_method(D_METHOD("set_voxel_scale", "scale"), &WaveFunctionCollapseGenerator::set_voxel_scale);
        ClassDB::bind_method(D_METHOD("get_voxel_scale"), &WaveFunctionCollapseGenerator::get_voxel_scale);  
        ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "voxel_scale"), "set_voxel_scale", "get_voxel_scale");

        ClassDB::bind_method(D_METHOD("set_target_grid_size", "size"), &WaveFunctionCollapseGenerator::set_target_grid_size);
        ClassDB::bind_method(D_METHOD("get_target_grid_size"), &WaveFunctionCollapseGenerator::get_target_grid_size);
        ADD_PROPERTY(PropertyInfo(Variant::VECTOR3I, "target_grid_size"), "set_target_grid_size", "get_target_grid_size");
    }

protected:

    Vector3i target_grid_size = Vector3i(64, 64, 64);
    float voxel_scale = 1.0f;
};

#endif // WAVE_FUNCTION_COLLAPSE
