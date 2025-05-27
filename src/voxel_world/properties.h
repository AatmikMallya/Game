#ifndef VOXEL_WORLD_PROPERTIES_H
#define VOXEL_WORLD_PROPERTIES_H

#include <godot_cpp/variant/packed_byte_array.hpp>
#include <godot_cpp/variant/vector4.hpp>
#include "gdcs/include/gdcs.h"

namespace godot
{
    struct Brick { // we may add color to this for simple LOD
        int occupancy_count;      // amount of voxels in the brick; 0 means the brick is empty
        unsigned int voxel_data_pointer;  // index of the first voxel in the brick (voxels stored in Morton order)
    };

    struct Voxel {
        int data;
    };

    struct VoxelWorldProperties // match the struct on the gpu
    {
        VoxelWorldProperties() = default;
        VoxelWorldProperties(Vector3i grid_size, Vector3i brick_grid_size, float scale = 1.0f)
            : scale(scale)
        {
            this->grid_size = Vector4i(grid_size.x, grid_size.y, grid_size.z, 0);
            this->brick_grid_size = Vector4i(brick_grid_size.x, brick_grid_size.y, brick_grid_size.z, 0);
        };

        Vector4i grid_size;
        Vector4i brick_grid_size;
        float scale;

        PackedByteArray to_packed_byte_array()
        {
            PackedByteArray byte_array;
            byte_array.resize(sizeof(VoxelWorldProperties));
            std::memcpy(byte_array.ptrw(), this, sizeof(VoxelWorldProperties));
            return byte_array;
        }
    };

    void add_voxel_buffers(ComputeShader* shader, const RID &voxel_bricks, const RID &voxel_data, const RID &properties)
    {
        shader->add_existing_buffer(voxel_bricks, RenderingDevice::UNIFORM_TYPE_STORAGE_BUFFER, 0, 0);
        shader->add_existing_buffer(voxel_data, RenderingDevice::UNIFORM_TYPE_STORAGE_BUFFER, 1, 0);
        shader->add_existing_buffer(properties, RenderingDevice::UNIFORM_TYPE_STORAGE_BUFFER, 2, 0);
    }
}

#endif // VOXEL_WORLD_PROPERTIES_H