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
            _grid_size = Vector4i(grid_size.x, grid_size.y, grid_size.z, 0);
            _brick_grid_size = Vector4i(brick_grid_size.x, brick_grid_size.y, brick_grid_size.z, 0);
        };

        void set_sky_colors(const Color &sky_color, const Color &ground_color) { 
            _sky_color = Vector4(sky_color.r, sky_color.g, sky_color.b, 0); 
            _ground_color = Vector4(ground_color.r, ground_color.g, ground_color.b, 0); 
        }

        void set_sun(const Color &sun_color, const Vector3 &sun_direction) { 
            _sun_color = Vector4(sun_color.r, sun_color.g, sun_color.b, 0); 
            _sun_direction = Vector4(sun_direction.x, sun_direction.y, sun_direction.z, 0); 
        }

        Vector4i _grid_size;
        Vector4i _brick_grid_size;
        Vector4 _sky_color;
        Vector4 _ground_color;
        Vector4 _sun_color;
        Vector4 _sun_direction;
        float scale;

        PackedByteArray to_packed_byte_array()
        {
            PackedByteArray byte_array;
            byte_array.resize(sizeof(VoxelWorldProperties));
            std::memcpy(byte_array.ptrw(), this, sizeof(VoxelWorldProperties));
            return byte_array;
        }
    };

    void add_voxel_buffers(ComputeShader* shader, const RID &voxel_bricks, const RID &voxel_data, const RID &properties);
}

#endif // VOXEL_WORLD_PROPERTIES_H