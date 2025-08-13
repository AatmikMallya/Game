#ifndef VOXEL_DATA_VOX_H
#define VOXEL_DATA_VOX_H

#include <godot_cpp/classes/node3d.hpp>
#include <godot_cpp/classes/file_access.hpp>
#include <godot_cpp/classes/rendering_device.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/rid.hpp>

#include "gdcs/include/gdcs.h"
#include "voxel_world/voxel_properties.h"
#include "voxel_data.h"

using namespace godot;

class VoxelDataVox : public VoxelData
{
    GDCLASS(VoxelDataVox, VoxelData)

  public:
    VoxelDataVox() {};
    ~VoxelDataVox() {};

    Vector3i get_size() const override;
    std::vector<Voxel> get_voxels() const override; //get all voxels in a grid linearized into x, y, z order

    Ref<FileAccess> get_file() const {
        return file;
    }
    void set_file(const Ref<FileAccess> &p_file) {
        file = p_file;
    }

    static void _bind_methods() {};

  private:
    Ref<FileAccess> file;
};

#endif // VOXEL_DATA_VOX_H
