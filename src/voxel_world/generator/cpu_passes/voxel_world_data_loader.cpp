#include "voxel_world_data_loader.h"

bool VoxelWorldDataLoader::generate(std::vector<Voxel> &voxels, const Vector3i bounds_min, const Vector3i bounds_max, const VoxelWorldProperties &properties)
{
    auto grid_size = (bounds_max - bounds_min);

    if(voxel_data.is_null()) {
        UtilityFunctions::printerr("VoxelData is not set in VoxelWorldDataLoader. Cannot load voxel data.");
        return false;
    }

    voxel_data->load();
    Vector3i size = (Vector3(voxel_data->get_size()) * voxel_scale).ceil();
    size = size.min(grid_size);

    for (int x = 0; x < size.x; ++x) {
        for (int y = 0; y < size.y; ++y) {
            for (int z = 0; z < size.z; ++z) {
                Voxel voxel = voxel_data->get_voxel_at((Vector3(x, y, z) / voxel_scale).floor());
                
                unsigned int voxel_index = properties.pos_to_voxel_index(Vector3i(x, y, z) + bounds_min);
                
                voxels[voxel_index] = voxel;
            }
        }
    }

    return true;

    // voxel_world_rids.set_voxel_data(voxel_data_vector);
}
