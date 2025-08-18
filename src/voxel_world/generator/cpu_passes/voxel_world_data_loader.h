#ifndef VOXEL_WORLD_DATA_LOADER_H
#define VOXEL_WORLD_DATA_LOADER_H

#include "voxel_world/generator/voxel_world_generator.h"
#include <godot_cpp/classes/node3d.hpp>
#include <godot_cpp/classes/rendering_device.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/rid.hpp>

#include "voxel_world/data/voxel_data.h"
#include "../voxel_world_generator_cpu_pass.h"

using namespace godot;

class VoxelWorldDataLoader : public VoxelWorldGeneratorCPUPass {
    GDCLASS(VoxelWorldDataLoader, VoxelWorldGeneratorCPUPass)

public:
    VoxelWorldDataLoader() = default;
    ~VoxelWorldDataLoader() override = default;

    void set_voxel_data(const Ref<VoxelData>& data) { voxel_data = data; }
    Ref<VoxelData> get_voxel_data() const { return voxel_data; }

    void set_voxel_scale(float scale) { voxel_scale = scale; }
    float get_voxel_scale() const { return voxel_scale; }

    bool generate(std::vector<Voxel> &voxel_data, const Vector3i bounds_min, const Vector3i bounds_max, const VoxelWorldProperties &properties) override;

    static void _bind_methods() {
        ClassDB::bind_method(D_METHOD("set_voxel_data", "path"), &VoxelWorldDataLoader::set_voxel_data);
        ClassDB::bind_method(D_METHOD("get_voxel_data"), &VoxelWorldDataLoader::get_voxel_data);
        ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "voxel_data", PROPERTY_HINT_RESOURCE_TYPE, "VoxelData"), "set_voxel_data", "get_voxel_data");

        ClassDB::bind_method(D_METHOD("set_voxel_scale", "scale"), &VoxelWorldDataLoader::set_voxel_scale);
        ClassDB::bind_method(D_METHOD("get_voxel_scale"), &VoxelWorldDataLoader::get_voxel_scale);  
        ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "voxel_scale"), "set_voxel_scale", "get_voxel_scale");
    }

private:
    Ref<VoxelData> voxel_data;
    float voxel_scale = 1.0f;
};

#endif // VOXEL_WORLD_DATA_LOADER_H
