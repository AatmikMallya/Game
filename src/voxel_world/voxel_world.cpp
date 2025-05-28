
#include "voxel_world.h"
#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/classes/rendering_server.hpp>
#include <godot_cpp/variant/utility_functions.hpp>
#include "voxel_world_generator.h"


using namespace godot;

VoxelWorld::VoxelWorld() {
    brick_map_size = Vector3i(16, 16, 16);
    scale = 0.125f;
}

VoxelWorld::~VoxelWorld() {
    // Cleanup code if necessary.
}

void VoxelWorld::edit_world(const Vector3 &camera_origin, const Vector3 &camera_direction, const float radius,
                            const float range)
{
    if(_edit_pass == nullptr) return;
    _edit_pass->edit_using_raycast(camera_origin, camera_direction, radius, range);
}

void VoxelWorld::_bind_methods() {
    // Bind the property accessor methods.
    ClassDB::bind_method(D_METHOD("get_brick_map_size"), &VoxelWorld::get_brick_map_size);
    ClassDB::bind_method(D_METHOD("set_brick_map_size", "brick_map_size"), &VoxelWorld::set_brick_map_size);
    ADD_PROPERTY(PropertyInfo(Variant::VECTOR3I, "brick_map_size"), "set_brick_map_size", "get_brick_map_size");

    // ClassDB::bind_method(D_METHOD("get_scale"), &VoxelWorld::get_scale);
    // ClassDB::bind_method(D_METHOD("set_scale", "scale"), &VoxelWorld::set_scale);
    // ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "scale"), "set_scale", "get_scale");

    ClassDB::bind_method(D_METHOD("get_simulation_enabled"), &VoxelWorld::get_simulation_enabled);
    ClassDB::bind_method(D_METHOD("set_simulation_enabled", "enabled"), &VoxelWorld::set_simulation_enabled);
    ADD_PROPERTY(PropertyInfo(Variant::BOOL, "simulation_enabled"), "set_simulation_enabled", "get_simulation_enabled");


    //methods
    ClassDB::bind_method(D_METHOD("edit_world", "camera_origin", "camera_direction", "radius", "range"), &VoxelWorld::edit_world);
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
        break;
    }
    case NOTIFICATION_INTERNAL_PHYSICS_PROCESS: {
        float delta = get_physics_process_delta_time();
        update(delta);
        break;
    }
    }
}

void VoxelWorld::init() {
    Vector3i size = brick_map_size * BRICK_SIZE;    

    _voxel_properties = VoxelWorldProperties(size, brick_map_size, scale);
    _rd = RenderingServer::get_singleton()->get_rendering_device();

    //create grid buffer
    PackedByteArray voxel_bricks;
    int brick_count = brick_map_size.x * brick_map_size.y * brick_map_size.z;
    voxel_bricks.resize(brick_count * sizeof(Brick));
    _voxel_bricks_rid = _rd->storage_buffer_create(voxel_bricks.size(), voxel_bricks);

    // Create the voxel data buffer.
    PackedByteArray voxel_data;
    int voxel_count = size.x * size.y * size.z;
    voxel_data.resize(voxel_count * sizeof(Voxel));
    _voxel_data_rid = _rd->storage_buffer_create(voxel_data.size(), voxel_data);

    // Create the voxel properties buffer. 
    PackedByteArray properties_data = _voxel_properties.to_packed_byte_array();
    _voxel_properties_rid = _rd->storage_buffer_create(properties_data.size(), properties_data);

    // // Create the voxel world generator.
    VoxelWorldGenerator generator;
    generator.initialize_brick_grid(_rd, _voxel_bricks_rid, _voxel_data_rid, _voxel_properties_rid, brick_map_size);
    generator.populate(_rd, _voxel_bricks_rid, _voxel_data_rid, _voxel_properties_rid, size);


    // Create the update pass.
    _update_pass = new VoxelWorldUpdatePass("res://addons/voxel_playground/src/shaders/automata.glsl", _rd, _voxel_bricks_rid, _voxel_data_rid, _voxel_properties_rid, size);


    // Create the edit pass.
    _edit_pass = new VoxelEditPass("res://addons/voxel_playground/src/shaders/voxel_edit/sphere_edit.glsl", _rd, _voxel_bricks_rid, _voxel_data_rid, _voxel_properties_rid, size);
}

void VoxelWorld::update(float delta) {
    if(simulation_enabled) {
        _update_pass->update(delta);
    }    
}




