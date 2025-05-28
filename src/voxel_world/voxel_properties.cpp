#include "voxel_properties.h"

void godot::add_voxel_buffers(ComputeShader *shader, const RID &voxel_bricks, const RID &voxel_data,
                              const RID &properties)
{
    
    shader->add_existing_buffer(voxel_bricks, RenderingDevice::UNIFORM_TYPE_STORAGE_BUFFER, 0, 0);
    shader->add_existing_buffer(voxel_data, RenderingDevice::UNIFORM_TYPE_STORAGE_BUFFER, 1, 0);
    shader->add_existing_buffer(properties, RenderingDevice::UNIFORM_TYPE_STORAGE_BUFFER, 2, 0);
}