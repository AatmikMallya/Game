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
        static const int BRICK_SIZE = 8;

        VoxelWorldProperties() = default;
        VoxelWorldProperties(Vector3i _grid_size, Vector3i _brick_grid_size, float _scale = 1.0f)
            : scale(_scale)
        {
            grid_size = Vector4i(_grid_size.x, _grid_size.y, _grid_size.z, 0);
            brick_grid_size = Vector4i(_brick_grid_size.x, _brick_grid_size.y, _brick_grid_size.z, 0);
        };

        void set_sky_colors(const Color &_sky_color, const Color &_ground_color) { 
            sky_color = Vector4(_sky_color.r, _sky_color.g, _sky_color.b, 0); 
            ground_color = Vector4(_ground_color.r, _ground_color.g, _ground_color.b, 0); 
        }

        void set_sun(const Color &_sun_color, const Vector3 &_sun_direction) { 
            sun_color = Vector4(_sun_color.r, _sun_color.g, _sun_color.b, 0); 
            sun_direction = Vector4(_sun_direction.x, _sun_direction.y, _sun_direction.z, 0); 
        }

        Vector4i grid_size;
        Vector4i brick_grid_size;
        Vector4 sky_color;
        Vector4 ground_color;
        Vector4 sun_color;
        Vector4 sun_direction;
        float scale;
        unsigned int frame;

        PackedByteArray to_packed_byte_array()
        {
            PackedByteArray byte_array;
            byte_array.resize(sizeof(VoxelWorldProperties));
            std::memcpy(byte_array.ptrw(), this, sizeof(VoxelWorldProperties));
            return byte_array;
        }

        int get_brick_index() const {
            return 0;
        }

        int get_voxel_location(const Vector3i &position) const
        {
            return position.x + position.y * grid_size.x + position.z * grid_size.x * grid_size.y;
        }
    };

    struct VoxelWorldRIDs {
        RID properties;
        RID voxel_bricks;
        RID voxel_data;
        RID voxel_data2;

        void add_voxel_buffers(ComputeShader* shader);
    };
}

#endif // VOXEL_WORLD_PROPERTIES_H