#ifndef VOXEL_WORLD_WFC_PATTERN_GENERATOR_H
#define VOXEL_WORLD_WFC_PATTERN_GENERATOR_H

#include "voxel_world/generator/voxel_world_generator.h"
#include <godot_cpp/classes/node3d.hpp>
#include <godot_cpp/classes/rendering_device.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/rid.hpp>

#include "voxel_world/data/voxel_data.h"
#include "wave_function_collapse_generator.h"

using namespace godot;

class VoxelWorldWFCPatternGenerator : public WaveFunctionCollapseGenerator {
    GDCLASS(VoxelWorldWFCPatternGenerator, WaveFunctionCollapseGenerator)

public:
    VoxelWorldWFCPatternGenerator() = default;
    ~VoxelWorldWFCPatternGenerator() override = default;

    void set_voxel_data(const Ref<VoxelData>& data) { voxel_data = data; }
    Ref<VoxelData> get_voxel_data() const { return voxel_data; }

    void set_seed_position_normalized(const Vector3 &pos) { seed_position_normalized = pos; }
    Vector3 get_seed_position_normalized() const { return seed_position_normalized; }

    void set_enable_superposition_propagation(bool enable) { enable_superposition_propagation = enable; }
    bool get_enable_superposition_propagation() const { return enable_superposition_propagation; }

    void set_show_contradictions(bool show) { show_contradictions = show; }
    bool get_show_contradictions() const { return show_contradictions; }

    void set_only_replace_air(bool only_air) { only_replace_air = only_air; }
    bool get_only_replace_air() const { return only_replace_air; }

    void set_initial_state(const Ref<VoxelWorldGeneratorCPUPass>& p_initial_state) { initial_state = p_initial_state; }
    Ref<VoxelWorldGeneratorCPUPass> get_initial_state() const { return initial_state; }

    bool generate(std::vector<Voxel> &result_voxels, const Vector3i bounds_min, const Vector3i bounds_max, const VoxelWorldProperties &properties) override;

    static void _bind_methods() {
        ClassDB::bind_method(D_METHOD("set_voxel_data", "data"), &VoxelWorldWFCPatternGenerator::set_voxel_data);
        ClassDB::bind_method(D_METHOD("get_voxel_data"), &VoxelWorldWFCPatternGenerator::get_voxel_data);
        ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "voxel_data", PROPERTY_HINT_RESOURCE_TYPE, "VoxelData"), "set_voxel_data", "get_voxel_data");

        ClassDB::bind_method(D_METHOD("set_seed_position_normalized", "pos"), &VoxelWorldWFCPatternGenerator::set_seed_position_normalized);
        ClassDB::bind_method(D_METHOD("get_seed_position_normalized"), &VoxelWorldWFCPatternGenerator::get_seed_position_normalized);
        ADD_PROPERTY(PropertyInfo(Variant::VECTOR3, "seed_position_normalized"), "set_seed_position_normalized", "get_seed_position_normalized");

        ClassDB::bind_method(D_METHOD("set_enable_superposition_propagation", "enable"), &VoxelWorldWFCPatternGenerator::set_enable_superposition_propagation);
        ClassDB::bind_method(D_METHOD("get_enable_superposition_propagation"), &VoxelWorldWFCPatternGenerator::get_enable_superposition_propagation);
        ADD_PROPERTY(PropertyInfo(Variant::BOOL, "enable_superposition_propagation"), "set_enable_superposition_propagation", "get_enable_superposition_propagation");

        ClassDB::bind_method(D_METHOD("set_show_contradictions", "show"), &VoxelWorldWFCPatternGenerator::set_show_contradictions);
        ClassDB::bind_method(D_METHOD("get_show_contradictions"), &VoxelWorldWFCPatternGenerator::get_show_contradictions);
        ADD_PROPERTY(PropertyInfo(Variant::BOOL, "show_contradictions"), "set_show_contradictions", "get_show_contradictions");

        ClassDB::bind_method(D_METHOD("set_only_replace_air", "only_air"), &VoxelWorldWFCPatternGenerator::set_only_replace_air);
        ClassDB::bind_method(D_METHOD("get_only_replace_air"), &VoxelWorldWFCPatternGenerator::get_only_replace_air);
        ADD_PROPERTY(PropertyInfo(Variant::BOOL, "only_replace_air"), "set_only_replace_air", "get_only_replace_air");

        ClassDB::bind_method(D_METHOD("set_initial_state", "initial_state"), &VoxelWorldWFCPatternGenerator::set_initial_state);
        ClassDB::bind_method(D_METHOD("get_initial_state"), &VoxelWorldWFCPatternGenerator::get_initial_state);
        ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "initial_state", PROPERTY_HINT_RESOURCE_TYPE, "VoxelWorldGeneratorCPUPass"), "set_initial_state", "get_initial_state");
    }

private:
    Ref<VoxelData> voxel_data;

    Vector3 seed_position_normalized = Vector3(0.5, 0.5, 0.5);
    bool enable_superposition_propagation = true;
    bool show_contradictions = true;
    bool only_replace_air = true;

    Ref<VoxelWorldGeneratorCPUPass> initial_state;
};

#endif // VOXEL_WORLD_WFC_PATTERN_GENERATOR_H
