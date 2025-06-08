#include "voxel_properties.h"

void godot::VoxelWorldRIDs::add_voxel_buffers(ComputeShader *shader)
{
    shader->add_existing_buffer(properties, RenderingDevice::UNIFORM_TYPE_STORAGE_BUFFER, 0, 0);
    shader->add_existing_buffer(voxel_bricks, RenderingDevice::UNIFORM_TYPE_STORAGE_BUFFER, 1, 0);
    shader->add_existing_buffer(voxel_data, RenderingDevice::UNIFORM_TYPE_STORAGE_BUFFER, 2, 0);
    shader->add_existing_buffer(voxel_data2, RenderingDevice::UNIFORM_TYPE_STORAGE_BUFFER, 3, 0);
}
