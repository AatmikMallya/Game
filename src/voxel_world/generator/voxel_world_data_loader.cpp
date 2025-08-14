#include "voxel_world_data_loader.h"

void VoxelWorldDataLoader::generate(RenderingDevice *rd, VoxelWorldRIDs &voxel_world_rids,
                                    const VoxelWorldProperties &properties)
{
    std::vector<Voxel> voxel_data_vector = std::vector<Voxel>(voxel_world_rids.voxel_count, Voxel::create_air_voxel());

    if(voxel_data.is_null()) {
        UtilityFunctions::printerr("VoxelData is not set in VoxelWorldDataLoader. Cannot load voxel data.");
        return;
    }

    voxel_data->load();
    Vector3i size = (Vector3(voxel_data->get_size()) * voxel_scale).ceil();
    // Vector3i size = Vector3i(128, 128, 128);

    size = size.min(Vector3i(properties.grid_size.x, properties.grid_size.y, properties.grid_size.z));

    for (int x = 0; x < size.x; ++x) {
        for (int y = 0; y < size.y; ++y) {
            for (int z = 0; z < size.z; ++z) {
                Voxel voxel = voxel_data->get_voxel_at((Vector3(x, y, z) / voxel_scale).floor());
                unsigned int voxel_index = properties.posToVoxelIndex(Vector3i(x, y, z));
                
                voxel_data_vector[voxel_index] = voxel;
            }
        }
    }

    voxel_world_rids.set_voxel_data(voxel_data_vector);
}