#include "voxel_camera.h"
#include "utility/utils.h"
#include <godot_cpp/classes/scene_tree.hpp>

void VoxelCamera::_bind_methods()
{
    ClassDB::bind_method(D_METHOD("get_fov"), &VoxelCamera::get_fov);
    ClassDB::bind_method(D_METHOD("set_fov", "value"), &VoxelCamera::set_fov);
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "fov"), "set_fov", "get_fov");

    ClassDB::bind_method(D_METHOD("get_voxel_world"), &VoxelCamera::get_voxel_world);
    ClassDB::bind_method(D_METHOD("set_voxel_world", "value"), &VoxelCamera::set_voxel_world);
    ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "voxel_world", PROPERTY_HINT_NODE_TYPE, "VoxelWorld"),
                 "set_voxel_world", "get_voxel_world");

    ClassDB::bind_method(D_METHOD("get_output_texture"), &VoxelCamera::get_output_texture);
    ClassDB::bind_method(D_METHOD("set_output_texture", "value"), &VoxelCamera::set_output_texture);
    ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "output_texture", PROPERTY_HINT_NODE_TYPE, "TextureRect"),
                 "set_output_texture", "get_output_texture");

    // Performance profiling methods
    ClassDB::bind_method(D_METHOD("get_time_camera_update"), &VoxelCamera::get_time_camera_update);
    ClassDB::bind_method(D_METHOD("get_time_raymarching"), &VoxelCamera::get_time_raymarching);
    ClassDB::bind_method(D_METHOD("get_time_total_render"), &VoxelCamera::get_time_total_render);

    // GPU timing methods
    ClassDB::bind_method(D_METHOD("get_gpu_time_raymarching"), &VoxelCamera::get_gpu_time_raymarching);

    // Projectile API
    ClassDB::bind_method(D_METHOD("register_projectile", "position", "radius"), &VoxelCamera::register_projectile);
    ClassDB::bind_method(D_METHOD("update_projectile", "id", "position", "radius"), &VoxelCamera::update_projectile);
    ClassDB::bind_method(D_METHOD("remove_projectile", "id"), &VoxelCamera::remove_projectile);

    // Entity API
    ClassDB::bind_method(D_METHOD("register_entity", "transform", "aabb_size", "scale"), &VoxelCamera::register_entity);
    ClassDB::bind_method(D_METHOD("update_entity", "id", "transform"), &VoxelCamera::update_entity);
    ClassDB::bind_method(D_METHOD("remove_entity", "id"), &VoxelCamera::remove_entity);
}

void VoxelCamera::_notification(int p_what)
{
    if (godot::Engine::get_singleton()->is_editor_hint())
    {
        return;
    }
    switch (p_what)
    {
    case NOTIFICATION_ENTER_TREE: {
        set_process_internal(true);

        break;
    }
    case NOTIFICATION_EXIT_TREE: {
        set_process_internal(false);

        break;
    }
    case NOTIFICATION_READY: {
        // Defer initialization until first process, so NodePath properties resolve
        // and children are ready in the tree.
        // We also attempt lazy init in INTERNAL_PROCESS when needed.
        break;
    }
    case NOTIFICATION_INTERNAL_PROCESS: {
        if (cs == nullptr) {
            init();
        }
        render();
        break;
    }
    }
}

float VoxelCamera::get_fov() const
{
    return fov;
}

void VoxelCamera::set_fov(float value)
{
    fov = value;
}

// int VoxelCamera::get_num_bounces() const
// {
//     return num_bounces;
// }

// void VoxelCamera::set_num_bounces(int value)
// {
//     num_bounces = value;
// }

TextureRect *VoxelCamera::get_output_texture() const
{
    return output_texture_rect;
}

void VoxelCamera::set_output_texture(TextureRect *value)
{
    output_texture_rect = value;
}

VoxelWorld *VoxelCamera::get_voxel_world() const
{
    return voxel_world;
}

void VoxelCamera::set_voxel_world(VoxelWorld *value)
{
    voxel_world = value;
}

void VoxelCamera::init()
{
    if (voxel_world == nullptr)
    {
        // Try to find a VoxelWorld in the scene to be resilient
        SceneTree *tree = get_tree();
        if (tree)
        {
            Node *node = tree->get_first_node_in_group("voxel_world");
            if (node)
            {
                VoxelWorld *vw = Object::cast_to<VoxelWorld>(node);
                if (vw)
                {
                    voxel_world = vw;
                }
            }
        }
        if (voxel_world == nullptr)
        {
            UtilityFunctions::printerr("No voxel world set.");
            return;
        }
    }

    _rd = RenderingServer::get_singleton()->get_rendering_device();

    //get resolution
    Vector2i resolution = DisplayServer::get_singleton()->window_get_size();
    auto near = 0.01f;
    auto far = 1000.0f;    

    projection_matrix = Projection::create_perspective(fov, static_cast<float>(resolution.width) / resolution.height, near, far, false);

    // setup compute shader
    cs = new ComputeShader("res://addons/voxel_playground/src/shaders/voxel_renderer_new.glsl", _rd, {"#define TESTe"});

    //--------- Voxel BUFFERS ---------    
    voxel_world->get_voxel_world_rids().add_voxel_buffers(cs);    

    //--------- GENERAL BUFFERS ---------
    { // input general buffer
        render_parameters.width = resolution.x;
        render_parameters.height = resolution.y;
        render_parameters.fov = fov;

        render_parameters_rid = cs->create_storage_buffer_uniform(render_parameters.to_packed_byte_array(), 2, 1);
    }

    { //camera buffer        
        Vector3 camera_position = get_global_transform().get_origin();
        Projection VP = projection_matrix * get_global_transform().affine_inverse();
        Projection IVP = VP.inverse();

        Utils::projection_to_float(camera_parameters.vp, VP);
        Utils::projection_to_float(camera_parameters.ivp, IVP);
        camera_parameters.cameraPosition = Vector4(camera_position.x, camera_position.y, camera_position.z, 1.0f);
        camera_parameters.frame_index = 0;
        camera_parameters.nearPlane = near;
        camera_parameters.farPlane = far;

        camera_parameters_rid = cs->create_storage_buffer_uniform(camera_parameters.to_packed_byte_array(), 3, 1);
    }

    //--------- PROJECTILE BUFFERS ---------
    {
        // Initialize CPU-side cache
        _projectiles.resize(MAX_PROJECTILES);
        for (int i = 0; i < MAX_PROJECTILES; ++i) {
            _projectiles.write[i].data = Vector4(0,0,0,0);
            _projectiles.write[i].active = false;
        }
        _projectile_params.projectile_count = 0;

        // GPU buffers: params + spheres array (vec4 per projectile)
        projectile_parameters_rid = cs->create_storage_buffer_uniform(_projectile_params.to_packed_byte_array(), 4, 1);

        PackedByteArray spheres;
        spheres.resize(sizeof(float) * 4 * MAX_PROJECTILES);
        spheres.fill(0);
        projectile_spheres_rid = cs->create_storage_buffer_uniform(spheres, 5, 1);
    }

    //--------- ENTITY BUFFERS ---------
    {
        // Initialize CPU-side cache
        _entities.resize(MAX_ENTITIES);
        for (int i = 0; i < MAX_ENTITIES; ++i) {
            _entities.write[i].active = false;
            // Zero out descriptor
            EntityDescriptor &desc = _entities.write[i].descriptor;
            memset(&desc, 0, sizeof(EntityDescriptor));
        }
        _entity_count.entity_count = 0;

        // GPU buffer: entity count
        entity_count_rid = cs->create_storage_buffer_uniform(_entity_count.to_packed_byte_array(), 6, 1);

        // GPU buffer: entity bricks (empty for now, will populate when entities have voxel data)
        PackedByteArray entity_bricks_data;
        entity_bricks_data.resize(1024); // Dummy size
        entity_bricks_data.fill(0);
        entity_bricks_rid = cs->create_storage_buffer_uniform(entity_bricks_data, 7, 1);

        // GPU buffer: entity voxels (empty for now)
        PackedByteArray entity_voxels_data;
        entity_voxels_data.resize(4096); // Dummy size
        entity_voxels_data.fill(0);
        entity_voxels_rid = cs->create_storage_buffer_uniform(entity_voxels_data, 8, 1);

        // GPU buffer: entity descriptors array
        PackedByteArray entity_descriptors_data;
        entity_descriptors_data.resize(sizeof(EntityDescriptor) * MAX_ENTITIES);
        entity_descriptors_data.fill(0);
        entity_descriptors_rid = cs->create_storage_buffer_uniform(entity_descriptors_data, 9, 1);
    }

    Ref<RDTextureView> output_texture_view = memnew(RDTextureView);
    { // output texture
        auto output_format = cs->create_texture_format(render_parameters.width, render_parameters.height, RenderingDevice::DATA_FORMAT_R32G32B32A32_SFLOAT);
        if (output_texture_rect == nullptr)
        {
            // Try to find a TextureRect child for convenience; fallback to headless rendering
            for (int i = 0; i < get_child_count(); ++i) {
                TextureRect *tr = Object::cast_to<TextureRect>(get_child(i));
                if (tr) { output_texture_rect = tr; break; }
            }
            if (output_texture_rect == nullptr)
            {
                UtilityFunctions::printerr("No output texture set. Rendering headless to RD texture.");
            }
        }
        output_image = Image::create(render_parameters.width, render_parameters.height, false, Image::FORMAT_RGBAF);
        output_texture_rid = cs->create_image_uniform(output_image, output_format, output_texture_view, 0, 1);

        if (output_texture_rect != nullptr) {
            output_texture.instantiate();
            output_texture->set_texture_rd_rid(output_texture_rid);
            output_texture_rect->set_texture(output_texture);
        }
    }

    Ref<RDTextureView> depth_texture_view = memnew(RDTextureView);
    { // depth texture
        auto depth_format = cs->create_texture_format(render_parameters.width, render_parameters.height, RenderingDevice::DATA_FORMAT_R32_SFLOAT);
        depth_image = Image::create(render_parameters.width, render_parameters.height, false, Image::FORMAT_RF);        
        depth_texture_rid = cs->create_image_uniform(depth_image, depth_format, depth_texture_view, 1, 1);
    }

    cs->finish_create_uniforms();
}

void VoxelCamera::clear_compute_shader()
{
}

void VoxelCamera::render()
{
    if (cs == nullptr || !cs->check_ready())
        return;

    uint64_t render_start = Time::get_singleton()->get_ticks_usec();

    // update rendering parameters
    uint64_t camera_update_start = Time::get_singleton()->get_ticks_usec();
    Vector3 camera_position = get_global_transform().get_origin();
    Projection VP = projection_matrix * get_global_transform().affine_inverse();
    Projection IVP = VP.inverse();

    Utils::projection_to_float(camera_parameters.vp, VP);
    Utils::projection_to_float(camera_parameters.ivp, IVP);
    camera_parameters.cameraPosition = Vector4(camera_position.x, camera_position.y, camera_position.z, 1.0f);
    camera_parameters.frame_index++;
    cs->update_storage_buffer_uniform(camera_parameters_rid, camera_parameters.to_packed_byte_array());
    uint64_t camera_update_end = Time::get_singleton()->get_ticks_usec();
    _time_camera_update_us = camera_update_end - camera_update_start;

    // Update projectile buffers
    {
        // Count active projectiles and pack data
        int count = 0;
        PackedByteArray spheres;
        spheres.resize(sizeof(float) * 4 * MAX_PROJECTILES);
        uint8_t *ptr = spheres.ptrw();

        for (int i = 0; i < MAX_PROJECTILES; ++i) {
            const ProjectileEntry &e = _projectiles[i];
            Vector4 v = e.active ? e.data : Vector4(0,0,0,0);
            std::memcpy(ptr + i * sizeof(float) * 4, &v, sizeof(float) * 4);
            if (e.active) count++;
        }
        _projectile_params.projectile_count = count;
        cs->update_storage_buffer_uniform(projectile_parameters_rid, _projectile_params.to_packed_byte_array());
        cs->update_storage_buffer_uniform(projectile_spheres_rid, spheres);
    }

    // Update entity buffers
    {
        // Count active entities and pack descriptors
        int count = 0;
        PackedByteArray descriptors_data;
        descriptors_data.resize(sizeof(EntityDescriptor) * MAX_ENTITIES);
        uint8_t *ptr = descriptors_data.ptrw();

        for (int i = 0; i < MAX_ENTITIES; ++i) {
            const EntityEntry &e = _entities[i];
            if (e.active) {
                std::memcpy(ptr + i * sizeof(EntityDescriptor), &e.descriptor, sizeof(EntityDescriptor));
                count++;
            } else {
                // Zero out inactive slots
                memset(ptr + i * sizeof(EntityDescriptor), 0, sizeof(EntityDescriptor));
            }
        }
        _entity_count.entity_count = count;

        // DEBUG: Print entity count every 60 frames
        static int debug_frame = 0;
        if (++debug_frame % 60 == 0 && count > 0) {
            UtilityFunctions::print("VoxelCamera render: entity_count=", count);
            for (int i = 0; i < MAX_ENTITIES; ++i) {
                if (_entities[i].active) {
                    UtilityFunctions::print("  Entity ", i, " enabled=", _entities[i].descriptor.enabled);
                }
            }
        }

        cs->update_storage_buffer_uniform(entity_count_rid, _entity_count.to_packed_byte_array());
        cs->update_storage_buffer_uniform(entity_descriptors_rid, descriptors_data);
    }

    // render
    uint64_t raymarching_start = Time::get_singleton()->get_ticks_usec();
    Vector2i Size = {render_parameters.width, render_parameters.height};
    cs->compute({static_cast<int32_t>(std::ceil(Size.x / 32.0f)), static_cast<int32_t>(std::ceil(Size.y / 32.0f)), 1}, true);  // Enable sync for GPU timing
    uint64_t raymarching_end = Time::get_singleton()->get_ticks_usec();
    _time_raymarching_us = raymarching_end - raymarching_start;
    _gpu_time_raymarching_ms = cs->get_last_gpu_time_ms();

    { // post processing

    }

    // output_image->set_data(Size.x, Size.y, false, Image::FORMAT_RGBA8,
    //                        cs->get_image_uniform_buffer(output_texture_rid));
    // output_texture->update(output_image);

    uint64_t render_end = Time::get_singleton()->get_ticks_usec();
    _time_total_render_us = render_end - render_start;
}

// ---------------- Projectile API ----------------
int VoxelCamera::register_projectile(const Vector3 &position, float radius)
{
    for (int i = 0; i < MAX_PROJECTILES; ++i) {
        if (!_projectiles[i].active) {
            _projectiles.write[i].active = true;
            _projectiles.write[i].data = Vector4(position.x, position.y, position.z, radius);
            return i;
        }
    }
    UtilityFunctions::push_warning("VoxelCamera: Max projectiles reached, cannot register more.");
    return -1;
}

void VoxelCamera::update_projectile(int id, const Vector3 &position, float radius)
{
    if (id < 0 || id >= MAX_PROJECTILES) return;
    if (!_projectiles[id].active) return;
    _projectiles.write[id].data = Vector4(position.x, position.y, position.z, radius);
}

void VoxelCamera::remove_projectile(int id)
{
    if (id < 0 || id >= MAX_PROJECTILES) return;
    _projectiles.write[id].active = false;
    _projectiles.write[id].data = Vector4(0,0,0,0);
}

// ---------------- Entity API ----------------
int VoxelCamera::register_entity(const Transform3D &transform, const Vector3 &aabb_size, float scale)
{
    for (int i = 0; i < MAX_ENTITIES; ++i) {
        if (!_entities[i].active) {
            _entities.write[i].active = true;
            EntityDescriptor &desc = _entities.write[i].descriptor;

            // Set transforms using proper transform_to_float utility
            Utils::transform_to_float(desc.local_to_world, transform);
            Transform3D inv_transform = transform.affine_inverse();
            Utils::transform_to_float(desc.world_to_local, inv_transform);

            // Set AABB
            desc.aabb_min = Vector4(-aabb_size.x/2, -aabb_size.y/2, -aabb_size.z/2, 0);
            desc.aabb_max = Vector4(aabb_size.x/2, aabb_size.y/2, aabb_size.z/2, 0);

            // For now, set dummy grid size (will populate with real voxel data later)
            desc.grid_size = Vector4(16, 16, 16, 0);
            desc.brick_grid_size = Vector4(2, 2, 2, 0);
            desc.scale = scale;
            desc.brick_offset = 0;
            desc.voxel_offset = 0;
            desc.brick_count = 0;
            desc.enabled = 1;
            desc.health = 100.0f;
            desc._pad0 = desc._pad1 = desc._pad2 = 0.0f;

            UtilityFunctions::print("VoxelCamera: Registered entity ", i,
                " at pos ", transform.origin,
                " with AABB min ", desc.aabb_min, " max ", desc.aabb_max,
                " enabled=", desc.enabled);

            // DEBUG: Print first few matrix values to verify
            UtilityFunctions::print("  local_to_world[0-3]: ", desc.local_to_world[0], ", ", desc.local_to_world[1], ", ", desc.local_to_world[2], ", ", desc.local_to_world[3]);
            UtilityFunctions::print("  world_to_local[0-3]: ", desc.world_to_local[0], ", ", desc.world_to_local[1], ", ", desc.world_to_local[2], ", ", desc.world_to_local[3]);

            return i;
        }
    }
    UtilityFunctions::push_warning("VoxelCamera: Max entities reached, cannot register more.");
    return -1;
}

void VoxelCamera::update_entity(int id, const Transform3D &transform)
{
    if (id < 0 || id >= MAX_ENTITIES) return;
    if (!_entities[id].active) return;

    EntityDescriptor &desc = _entities.write[id].descriptor;

    // Update transforms using proper transform_to_float utility
    Utils::transform_to_float(desc.local_to_world, transform);
    Transform3D inv_transform = transform.affine_inverse();
    Utils::transform_to_float(desc.world_to_local, inv_transform);
}

void VoxelCamera::remove_entity(int id)
{
    if (id < 0 || id >= MAX_ENTITIES) return;
    _entities.write[id].active = false;
    memset(&_entities.write[id].descriptor, 0, sizeof(EntityDescriptor));
}
