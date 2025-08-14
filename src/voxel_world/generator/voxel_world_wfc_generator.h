#ifndef VOXEL_WORLD_WFC_GENERATOR_H
#define VOXEL_WORLD_WFC_GENERATOR_H

#include "voxel_world/generator/voxel_world_generator.h"
#include <godot_cpp/classes/node3d.hpp>
#include <godot_cpp/classes/rendering_device.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/rid.hpp>

#include "voxel_world/data/voxel_data_vox.h"

using namespace godot;

class VoxelWorldWFCGenerator : public VoxelWorldGenerator {
    GDCLASS(VoxelWorldWFCGenerator, VoxelWorldGenerator)

public:
    VoxelWorldWFCGenerator() = default;
    ~VoxelWorldWFCGenerator() override = default;

    void set_voxel_data(const Ref<VoxelDataVox>& data) { voxel_data = data; }
    Ref<VoxelDataVox> get_voxel_data() const { return voxel_data; }

    // void set_voxel_scale(float scale) { voxel_scale = scale; }
    // float get_voxel_scale() const { return voxel_scale; }

    void generate(RenderingDevice* rd, VoxelWorldRIDs& voxel_world_rids, const VoxelWorldProperties& properties) override;

    static void _bind_methods() {
        ClassDB::bind_method(D_METHOD("set_voxel_data", "path"), &VoxelWorldWFCGenerator::set_voxel_data);
        ClassDB::bind_method(D_METHOD("get_voxel_data"), &VoxelWorldWFCGenerator::get_voxel_data);
        ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "voxel_data", PROPERTY_HINT_RESOURCE_TYPE, "VoxelDataVox"), "set_voxel_data", "get_voxel_data");

        // ClassDB::bind_method(D_METHOD("set_voxel_scale", "scale"), &VoxelWorldWFCGenerator::set_voxel_scale);
        // ClassDB::bind_method(D_METHOD("get_voxel_scale"), &VoxelWorldWFCGenerator::get_voxel_scale);  
        // ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "voxel_scale"), "set_voxel_scale", "get_voxel_scale");
    }

private:
    Ref<VoxelDataVox> voxel_data;
    Vector3i grid_size = Vector3i(64, 64, 64); // Default bounds for the WFC grid
    // float voxel_scale = 1.0f;
};

#endif // VOXEL_WORLD_WFC_GENERATOR_H
