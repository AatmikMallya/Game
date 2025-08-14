#ifndef VOXEL_DATA_VOX_H
#define VOXEL_DATA_VOX_H

#include <godot_cpp/classes/file_access.hpp>
#include <godot_cpp/classes/node3d.hpp>
#include <godot_cpp/classes/rendering_device.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/rid.hpp>

#include "gdcs/include/gdcs.h"
#include "voxel_data.h"
#include "voxel_world/voxel_properties.h"

using namespace godot;

class VoxelDataVox : public VoxelData
{
    GDCLASS(VoxelDataVox, VoxelData)

  public:
    VoxelDataVox() = default;
    ~VoxelDataVox() override = default;

    Vector3i get_size() const override
    {
        return size;
    }
    std::vector<Voxel> get_voxels() const override
    {
        return voxels;
    }
    std::vector<uint8_t> get_voxel_indices() const
    {
        return voxel_indices;
    }
    std::vector<Color> get_palette() const
    {
        return palette;
    }

    Voxel get_voxel_at(Vector3i p) const override
    {
        if (p.x < 0 || p.y < 0 || p.z < 0 || p.x >= size.x || p.y >= size.y || p.z >= size.z)
            return Voxel::create_air_voxel();
        if (swap_y_z)
        {
            int temp = p.y;
            p.y = p.z;
            p.z = temp;
        }

        size_t idx = ((size_t)p.z * size.y + (size_t)p.y) * size.x + (size_t)p.x;
        return voxels[idx];
    }

    uint8_t get_voxel_index_at(Vector3i p) const
    {
        if (p.x < 0 || p.y < 0 || p.z < 0 || p.x >= size.x || p.y >= size.y || p.z >= size.z)
            return 0;
        if (swap_y_z)
        {
            int temp = p.y;
            p.y = p.z;
            p.z = temp;
        }

        size_t idx = ((size_t)p.z * size.y + (size_t)p.y) * size.x + (size_t)p.x;
        return voxel_indices[idx];
    }

    String get_file_path() const
    {
        return file_path;
    }
    void set_file_path(const String &p_file_path)
    {
        file_path = p_file_path;
    }

    bool get_swap_y_z() const
    {
        return swap_y_z;
    }
    void set_swap_y_z(bool p_swap_y_z)
    {
        swap_y_z = p_swap_y_z;
    }

    Error load() override;

    static void _bind_methods()
    {
        ClassDB::bind_method(D_METHOD("get_file_path"), &VoxelDataVox::get_file_path);
        ClassDB::bind_method(D_METHOD("set_file_path", "file_path"), &VoxelDataVox::set_file_path);
        ADD_PROPERTY(PropertyInfo(Variant::STRING, "file_path", PROPERTY_HINT_FILE, "*.vox"), "set_file_path",
                     "get_file_path");

        ClassDB::bind_method(D_METHOD("get_swap_y_z"), &VoxelDataVox::get_swap_y_z);
        ClassDB::bind_method(D_METHOD("set_swap_y_z", "swap_y_z"), &VoxelDataVox::set_swap_y_z);
        ADD_PROPERTY(PropertyInfo(Variant::BOOL, "swap_y_z"), "set_swap_y_z", "get_swap_y_z");
    }

  private:
    String file_path;
    Vector3i size = Vector3i(0, 0, 0);
    std::vector<Color> palette;
    std::vector<Voxel> voxels;
    std::vector<uint8_t> voxel_indices;
    bool swap_y_z = true; // MagicaVoxel uses Z-up, Godot uses Y-up, so we need to swap Y and Z axes when loading
    static const uint32_t VoxelDataVox::DEFAULT_VOX_PALETTE_ABGR[256];
};

#endif // VOXEL_DATA_VOX_H
