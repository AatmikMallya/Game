
#include "voxel_world.h"
#include "voxel_world_generator.h"
#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/classes/rendering_server.hpp>
#include <godot_cpp/variant/utility_functions.hpp>

using namespace godot;

VoxelWorld::VoxelWorld()
{
    brick_map_size = Vector3i(16, 16, 16);
    scale = 0.125f;
    _initialized = false;
    _has_last_collider_pos = false;
    _has_last_collider_pos_aux = false;
}

VoxelWorld::~VoxelWorld()
{

}

void VoxelWorld::edit_world(const Vector3 &camera_origin, const Vector3 &camera_direction, const float radius,
                            const float range, const int value)
{
    if (_edit_pass == nullptr)
        return;
    _edit_pass->edit_using_raycast(camera_origin, camera_direction, radius, range, value);
}

void VoxelWorld::edit_sphere_at(const Vector3 &position, const float radius, const int value)
{
    if (_edit_pass == nullptr)
        return;
    // Convert world position (meters) to voxel grid coordinates before editing shader
    Vector3i grid = get_voxel_world_position(position);
    _edit_pass->edit_at(Vector3(grid.x, grid.y, grid.z), radius, value);
}

Vector4 VoxelWorld::raycast_voxels(const Vector3 &origin, const Vector3 &direction, float near, float far)
{
    if (_edit_pass == nullptr)
        return Vector4(0, 0, 0, -1);
    return _edit_pass->raycast_voxels(origin, direction, near, far);
}

void VoxelWorld::_bind_methods()
{
    ClassDB::bind_method(D_METHOD("get_generator"), &VoxelWorld::get_generator);
    ClassDB::bind_method(D_METHOD("set_generator", "generator"), &VoxelWorld::set_generator);
    ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "generator", PROPERTY_HINT_RESOURCE_TYPE, "VoxelWorldGenerator"),
                 "set_generator", "get_generator");

    ClassDB::bind_method(D_METHOD("get_brick_map_size"), &VoxelWorld::get_brick_map_size);
    ClassDB::bind_method(D_METHOD("set_brick_map_size", "brick_map_size"), &VoxelWorld::set_brick_map_size);
    ADD_PROPERTY(PropertyInfo(Variant::VECTOR3I, "brick_map_size"), "set_brick_map_size", "get_brick_map_size");

    ClassDB::bind_method(D_METHOD("get_scale"), &VoxelWorld::get_scale);
    ClassDB::bind_method(D_METHOD("set_scale", "scale"), &VoxelWorld::set_scale);
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "scale"), "set_scale", "get_scale");

    ClassDB::bind_method(D_METHOD("get_simulation_enabled"), &VoxelWorld::get_simulation_enabled);
    ClassDB::bind_method(D_METHOD("set_simulation_enabled", "enabled"), &VoxelWorld::set_simulation_enabled);
    ADD_PROPERTY(PropertyInfo(Variant::BOOL, "simulation_enabled"), "set_simulation_enabled", "get_simulation_enabled");

    ClassDB::bind_method(D_METHOD("set_voxel_world_collider", "collider"), &VoxelWorld::set_voxel_world_collider);
    ClassDB::bind_method(D_METHOD("get_voxel_world_collider"), &VoxelWorld::get_voxel_world_collider);
    ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "voxel_world_collider", PROPERTY_HINT_NODE_TYPE, "VoxelWorldCollider"),
                 "set_voxel_world_collider", "get_voxel_world_collider");
    ClassDB::bind_method(D_METHOD("set_voxel_world_collider_aux", "collider"), &VoxelWorld::set_voxel_world_collider_aux);
    ClassDB::bind_method(D_METHOD("get_voxel_world_collider_aux"), &VoxelWorld::get_voxel_world_collider_aux);
    ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "voxel_world_collider_aux", PROPERTY_HINT_NODE_TYPE, "VoxelWorldCollider"),
                 "set_voxel_world_collider_aux", "get_voxel_world_collider_aux");

    ClassDB::bind_method(D_METHOD("get_player_node"), &VoxelWorld::get_player_node);
    ClassDB::bind_method(D_METHOD("set_player_node", "player_node"), &VoxelWorld::set_player_node);
    ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "player_node", PROPERTY_HINT_NODE_TYPE, "Node3D"), "set_player_node",
                 "get_player_node");
    ClassDB::bind_method(D_METHOD("get_aux_node"), &VoxelWorld::get_aux_node);
    ClassDB::bind_method(D_METHOD("set_aux_node", "aux_node"), &VoxelWorld::set_aux_node);
    ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "aux_node", PROPERTY_HINT_NODE_TYPE, "Node3D"), "set_aux_node",
                 "get_aux_node");

    ClassDB::bind_method(D_METHOD("get_sun_light"), &VoxelWorld::get_sun_light);
    ClassDB::bind_method(D_METHOD("set_sun_light", "sun_light"), &VoxelWorld::set_sun_light);
    ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "sun_light", PROPERTY_HINT_NODE_TYPE, "DirectionalLight3D"),
                 "set_sun_light", "get_sun_light");

    ClassDB::bind_method(D_METHOD("get_ground_color"), &VoxelWorld::get_ground_color);
    ClassDB::bind_method(D_METHOD("set_ground_color", "ground_color"), &VoxelWorld::set_ground_color);
    ADD_PROPERTY(PropertyInfo(Variant::COLOR, "ground_color"), "set_ground_color", "get_ground_color");

    ClassDB::bind_method(D_METHOD("get_sky_color"), &VoxelWorld::get_sky_color);
    ClassDB::bind_method(D_METHOD("set_sky_color", "sky_color"), &VoxelWorld::set_sky_color);
    ADD_PROPERTY(PropertyInfo(Variant::COLOR, "sky_color"), "set_sky_color", "get_sky_color");

    // methods
    ClassDB::bind_method(D_METHOD("edit_world", "camera_origin", "camera_direction", "radius", "range", "value"),
                         &VoxelWorld::edit_world);
    ClassDB::bind_method(D_METHOD("edit_sphere_at", "position", "radius", "value"), &VoxelWorld::edit_sphere_at);
    ClassDB::bind_method(D_METHOD("raycast_voxels", "origin", "direction", "near", "far"), &VoxelWorld::raycast_voxels);

    // Performance profiling methods
    ClassDB::bind_method(D_METHOD("get_time_simulation_liquid"), &VoxelWorld::get_time_simulation_liquid);
    ClassDB::bind_method(D_METHOD("get_time_simulation_freeze"), &VoxelWorld::get_time_simulation_freeze);
    ClassDB::bind_method(D_METHOD("get_time_simulation_cleanup"), &VoxelWorld::get_time_simulation_cleanup);
    ClassDB::bind_method(D_METHOD("get_time_collision"), &VoxelWorld::get_time_collision);
    ClassDB::bind_method(D_METHOD("get_time_total_update"), &VoxelWorld::get_time_total_update);

    // GPU timing methods
    ClassDB::bind_method(D_METHOD("get_gpu_time_simulation_liquid"), &VoxelWorld::get_gpu_time_simulation_liquid);
    ClassDB::bind_method(D_METHOD("get_gpu_time_simulation_freeze"), &VoxelWorld::get_gpu_time_simulation_freeze);
    ClassDB::bind_method(D_METHOD("get_gpu_time_simulation_cleanup"), &VoxelWorld::get_gpu_time_simulation_cleanup);
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

void VoxelWorld::init()
{
    Vector3i size = brick_map_size * BRICK_SIZE;

    _voxel_properties = VoxelWorldProperties(size, brick_map_size, scale);
    _voxel_properties.set_sky_colors(sky_color, ground_color);
    if (_sun_light != nullptr)
        _voxel_properties.set_sun(_sun_light->get_color(), -_sun_light->get_global_transform().basis.rows[2]);
    _voxel_properties.frame = 0;
    _rd = RenderingServer::get_singleton()->get_rendering_device();
    _voxel_world_rids.rendering_device = _rd;

    // create grid buffer
    PackedByteArray voxel_bricks;
    int brick_count = brick_map_size.x * brick_map_size.y * brick_map_size.z;
    voxel_bricks.resize(brick_count * sizeof(Brick));
    _voxel_world_rids.voxel_bricks = _rd->storage_buffer_create(voxel_bricks.size(), voxel_bricks);
    _voxel_world_rids.brick_count = brick_count;

    // Create the voxel data buffer.
    PackedByteArray voxel_data;
    int voxel_count = size.x * size.y * size.z;
    if (voxel_count * sizeof(Voxel) > 4.0e9f)
    {
        UtilityFunctions::printerr(
            "VoxelWorld: The voxel world is too large (exceeds 4GB, or 2 billion voxels). Reduce the brick map size.");
        return;
    }
    voxel_data.resize(voxel_count * sizeof(Voxel));
    _voxel_world_rids.voxel_data = _rd->storage_buffer_create(voxel_data.size(), voxel_data);
    _voxel_world_rids.voxel_data2 = _rd->storage_buffer_create(voxel_data.size(), voxel_data); //create a second to facilitate ping-pong buffers
    _voxel_world_rids.voxel_count = voxel_count;

    // Create the voxel properties buffer.
    PackedByteArray properties_data = _voxel_properties.to_packed_byte_array();
    _voxel_world_rids.properties = _rd->storage_buffer_create(properties_data.size(), properties_data);

    if (generator.is_null())
    {
        UtilityFunctions::printerr(
            "VoxelWorld: No world generator set.");
        return;
    }
    generator->initialize_brick_grid(_rd, _voxel_world_rids, _voxel_properties);
    generator->generate(_rd, _voxel_world_rids, _voxel_properties);

    // Create the update pass.
    _update_pass = new VoxelWorldUpdatePass("res://addons/voxel_playground/src/shaders/automata/liquid.glsl", _rd, _voxel_world_rids, size);

    // Create the edit pass.
    _edit_pass = new VoxelEditPass("res://addons/voxel_playground/src/shaders/voxel_edit/sphere_edit.glsl", _rd, _voxel_world_rids, size);

    // if colliders set, initialize them
    if (_voxel_world_collider != nullptr)
    {
        _voxel_world_collider->init(_rd, _voxel_world_rids, scale);
    }
    if (_voxel_world_collider_aux != nullptr)
    {
        _voxel_world_collider_aux->init(_rd, _voxel_world_rids, scale);
    }
    
    _initialized = true;
}

void VoxelWorld::update(float delta)
{
    if(!_initialized)
        return;

    uint64_t update_start = Time::get_singleton()->get_ticks_usec();

    _voxel_properties.frame++;
    PackedByteArray properties_data = _voxel_properties.to_packed_byte_array();
    _rd->buffer_update(_voxel_world_rids.properties, 0, properties_data.size(), properties_data);

    if (simulation_enabled)
    {
        uint64_t sim_start = Time::get_singleton()->get_ticks_usec();
        _update_pass->update(delta);
        uint64_t sim_end = Time::get_singleton()->get_ticks_usec();

        // Get individual pass timings from update pass
        _time_simulation_liquid_us = _update_pass->get_time_liquid_us();
        _time_simulation_freeze_us = _update_pass->get_time_freeze_us();
        _time_simulation_cleanup_us = _update_pass->get_time_cleanup_us();
    }
    else
    {
        _time_simulation_liquid_us = 0;
        _time_simulation_freeze_us = 0;
        _time_simulation_cleanup_us = 0;
    }

    // Update primary collider at player
    if (_voxel_world_collider != nullptr && player_node != nullptr)
    {
        // Snap collider update center to reduce remeshing churn when moving/falling.
        static const int SNAP_VOX = 4; // 4 voxels; with scale=0.25 this equals 1m
        Vector3 player_pos = player_node->get_global_position();
        Vector3i pvox = get_voxel_world_position(player_pos);

        auto snap_comp = [](int v) {
            // floor-like snapping that behaves for negative coordinates
            int q = v / SNAP_VOX;
            int r = v % SNAP_VOX;
            if (r < 0) {
                q -= 1;
            }
            return q * SNAP_VOX;
        };

        Vector3i snapped(snap_comp(pvox.x), snap_comp(pvox.y), snap_comp(pvox.z));

        if (!_has_last_collider_pos || snapped != _last_collider_voxel_pos)
        {
            uint64_t collision_start = Time::get_singleton()->get_ticks_usec();
            _voxel_world_collider->update(snapped);
            uint64_t collision_end = Time::get_singleton()->get_ticks_usec();
            _time_collision_us = collision_end - collision_start;

            _last_collider_voxel_pos = snapped;
            _has_last_collider_pos = true;
        }
    }
    // Update auxiliary collider at aux_node (e.g., an enemy like slime)
    if (_voxel_world_collider_aux != nullptr && (aux_node_id != ObjectID()))
    {
        static const int SNAP_VOX = 4;
        // Resolve aux node safely via ObjectDB each frame in case it was freed.
        Object *aux_obj = ObjectDB::get_instance((uint64_t)aux_node_id);
        Node3D *aux = Object::cast_to<Node3D>(aux_obj);
        if (aux == nullptr) {
            aux_node = nullptr;
            aux_node_id = ObjectID();
        } else {
            Vector3 aux_pos = aux->get_global_position();
        Vector3i avox = get_voxel_world_position(aux_pos);

        auto snap_comp2 = [](int v) {
            int q = v / SNAP_VOX;
            int r = v % SNAP_VOX;
            if (r < 0) {
                q -= 1;
            }
            return q * SNAP_VOX;
        };
        Vector3i snapped2(snap_comp2(avox.x), snap_comp2(avox.y), snap_comp2(avox.z));

            if (!_has_last_collider_pos_aux || snapped2 != _last_collider_voxel_pos_aux)
            {
                _voxel_world_collider_aux->update(snapped2);
                _last_collider_voxel_pos_aux = snapped2;
                _has_last_collider_pos_aux = true;
            }
        }
    }
    else
    {
        _time_collision_us = 0;
    }

    uint64_t update_end = Time::get_singleton()->get_ticks_usec();
    _time_total_update_us = update_end - update_start;
}

// GPU timing implementations
float VoxelWorld::get_gpu_time_simulation_liquid() const
{
    if (_update_pass == nullptr)
        return 0.0f;
    return _update_pass->get_gpu_time_liquid_ms();
}

float VoxelWorld::get_gpu_time_simulation_freeze() const
{
    if (_update_pass == nullptr)
        return 0.0f;
    return _update_pass->get_gpu_time_freeze_ms();
}

float VoxelWorld::get_gpu_time_simulation_cleanup() const
{
    if (_update_pass == nullptr)
        return 0.0f;
    return _update_pass->get_gpu_time_cleanup_ms();
}
