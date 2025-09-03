#include "voxel_world_cpu_generator.h"

using namespace godot;

void VoxelWorldCPUGenerator::generate(RenderingDevice *rd, VoxelWorldRIDs &voxel_world_rids,
                                      const VoxelWorldProperties &properties)
{
    if(pass.is_null()) {
        UtilityFunctions::printerr("No generator set, build cancelled.");
        return;
    }
    auto voxel_volume = Vector3i(properties.brick_grid_size.x, properties.brick_grid_size.y, properties.brick_grid_size.z) * properties.BRICK_SIZE;

    int N = voxel_world_rids.voxel_count;
    std::vector<Voxel> voxels = std::vector<Voxel>(N, Voxel::create_air_voxel());
    bool success = pass->generate(voxels, Vector3i(0,0,0), voxel_volume, properties);
    voxel_world_rids.set_voxel_data(voxels);
}