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

    void generate(RenderingDevice* rd, VoxelWorldRIDs& voxel_world_rids, const VoxelWorldProperties& properties) override;

    static void _bind_methods() {
        ClassDB::bind_method(D_METHOD("set_voxel_data", "path"), &VoxelWorldWFCPatternGenerator::set_voxel_data);
        ClassDB::bind_method(D_METHOD("get_voxel_data"), &VoxelWorldWFCPatternGenerator::get_voxel_data);
        ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "voxel_data", PROPERTY_HINT_RESOURCE_TYPE, "VoxelData"), "set_voxel_data", "get_voxel_data");
    }

private:
    Ref<VoxelData> voxel_data;
};

#endif // VOXEL_WORLD_WFC_PATTERN_GENERATOR_H
