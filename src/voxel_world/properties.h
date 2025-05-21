#ifndef VOXEL_WORLD_PROPERTIES_H
#define VOXEL_WORLD_PROPERTIES_H

#include <godot_cpp/variant/packed_byte_array.hpp>
#include <godot_cpp/variant/vector4.hpp>

namespace godot
{
    struct Voxel {
        int data;
    };

    struct VoxelWorldProperties // match the struct on the gpu
    {
        VoxelWorldProperties() = default;
        VoxelWorldProperties(Vector3i size, float scale = 1.0f)
            : scale(scale)
        {
            width = size.x;
            height = size.y;
            depth = size.z;
        };

        int width, height, depth;
        float scale;

        PackedByteArray to_packed_byte_array()
        {
            PackedByteArray byte_array;
            byte_array.resize(sizeof(VoxelWorldProperties));
            std::memcpy(byte_array.ptrw(), this, sizeof(VoxelWorldProperties));
            return byte_array;
        }
    };
}

#endif // VOXEL_WORLD_PROPERTIES_H