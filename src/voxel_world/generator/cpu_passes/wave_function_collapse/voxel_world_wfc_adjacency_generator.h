#ifndef VOXEL_WORLD_WFC_ADJACENCY_GENERATOR_H
#define VOXEL_WORLD_WFC_ADJACENCY_GENERATOR_H

#include "voxel_world/generator/voxel_world_generator.h"
#include <godot_cpp/classes/node3d.hpp>
#include <godot_cpp/classes/rendering_device.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/rid.hpp>

#include "voxel_world/data/voxel_data_vox.h"
#include "wave_function_collapse_generator.h"

using namespace godot;

class VoxelWorldWFCAdjacencyGenerator : public WaveFunctionCollapseGenerator {
    GDCLASS(VoxelWorldWFCAdjacencyGenerator, WaveFunctionCollapseGenerator)

public:
    VoxelWorldWFCAdjacencyGenerator() = default;
    ~VoxelWorldWFCAdjacencyGenerator() override = default;

    void set_voxel_data(const Ref<VoxelDataVox>& data) { voxel_data = data; }
    Ref<VoxelDataVox> get_voxel_data() const { return voxel_data; }

    bool generate(std::vector<Voxel> &result_voxels, const Vector3i bounds_min, const Vector3i bounds_max, const VoxelWorldProperties &properties) override;

    static void _bind_methods() {
        ClassDB::bind_method(D_METHOD("set_voxel_data", "path"), &VoxelWorldWFCAdjacencyGenerator::set_voxel_data);
        ClassDB::bind_method(D_METHOD("get_voxel_data"), &VoxelWorldWFCAdjacencyGenerator::get_voxel_data);
        ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "voxel_data", PROPERTY_HINT_RESOURCE_TYPE, "VoxelDataVox"), "set_voxel_data", "get_voxel_data");
    }

private:
    Ref<VoxelDataVox> voxel_data;
};

#endif // VOXEL_WORLD_WFC_ADJACENCY_GENERATOR_H
