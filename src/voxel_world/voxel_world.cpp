
#include "voxel_world.h"
#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/classes/rendering_server.hpp>
#include "voxel_world_generator.h"


using namespace godot;

VoxelWorld::VoxelWorld() {
    size = Vector3i(16, 16, 16);
    scale = 0.1f;
}

VoxelWorld::~VoxelWorld() {
    // Cleanup code if necessary.
}

void VoxelWorld::_bind_methods() {
    // Bind the property accessor methods.
    ClassDB::bind_method(D_METHOD("get_size"), &VoxelWorld::get_size);
    ClassDB::bind_method(D_METHOD("set_size", "size"), &VoxelWorld::set_size);
    ADD_PROPERTY(PropertyInfo(Variant::VECTOR3, "size"), "set_size", "get_size");

    ClassDB::bind_method(D_METHOD("get_scale"), &VoxelWorld::get_scale);
    ClassDB::bind_method(D_METHOD("set_scale", "scale"), &VoxelWorld::set_scale);
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "scale"), "set_scale", "get_scale");
}

void VoxelWorld::_notification(int p_what)
{
    if (godot::Engine::get_singleton()->is_editor_hint())
    {
        return;
    }
    switch (p_what)
    {
    case NOTIFICATION_ENTER_TREE: {
        set_physics_process_internal(true);

        break;
    }
    case NOTIFICATION_EXIT_TREE: {
        set_physics_process_internal(false);

        break;
    }
    case NOTIFICATION_READY: {
        init();
    }
    case NOTIFICATION_INTERNAL_PHYSICS_PROCESS: {
        float delta = get_physics_process_delta_time();
        update(delta);
        break;
    }
    }
}

void VoxelWorld::init() {
    _voxel_properties = VoxelWorldProperties(size, scale);
    _rd = RenderingServer::get_singleton()->get_rendering_device();

    // Create the voxel data buffer.
    PackedByteArray voxel_data;
    voxel_data.resize(size.x * size.y * size.z * sizeof(Voxel));
    _voxel_data_rid = _rd->storage_buffer_create(voxel_data.size(), voxel_data);

    // Create the voxel properties buffer. 
    PackedByteArray properties_data = _voxel_properties.to_packed_byte_array();
    _voxel_properties_rid = _rd->storage_buffer_create(properties_data.size(), properties_data);

    // // Create the voxel world generator.
    VoxelWorldGenerator generator;
    generator.populate(_rd, _voxel_data_rid, _voxel_properties_rid, size);
}

void VoxelWorld::update(float delta) {

}


