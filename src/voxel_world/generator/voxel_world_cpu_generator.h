#ifndef VOXEL_WORLD_CPU_GENERATOR_H
#define VOXEL_WORLD_CPU_GENERATOR_H

#include "voxel_world/generator/voxel_world_generator.h"
#include <godot_cpp/classes/node3d.hpp>
#include <godot_cpp/classes/rendering_device.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/rid.hpp>
#include "voxel_world_generator_cpu_pass.h"

using namespace godot;

class VoxelWorldCPUGenerator : public VoxelWorldGenerator {
    GDCLASS(VoxelWorldCPUGenerator, VoxelWorldGenerator)

protected:
    Ref<VoxelWorldGeneratorCPUPass> pass;

public:
    VoxelWorldCPUGenerator() = default;
    ~VoxelWorldCPUGenerator() override = default;

    void set_generator(const Ref<VoxelWorldGeneratorCPUPass> p_pass) { pass = p_pass; }
    Ref<VoxelWorldGeneratorCPUPass> get_generator() const { return pass; }

    void generate(RenderingDevice* rd, VoxelWorldRIDs& voxel_world_rids, const VoxelWorldProperties& properties) override;

    static void _bind_methods() {
        ClassDB::bind_method(D_METHOD("set_generator", "generator"), &VoxelWorldCPUGenerator::set_generator);
        ClassDB::bind_method(D_METHOD("get_generator"), &VoxelWorldCPUGenerator::get_generator);
        ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "generator", PROPERTY_HINT_RESOURCE_TYPE, "VoxelWorldGeneratorCPUPass"), "set_generator", "get_generator");
    }
};

#endif // VOXEL_WORLD_SHADER_GENERATOR_H
