#include "voxel_camera.h"
#include "utility/utils.h"
#include <godot_cpp/classes/scene_tree.hpp>
#include <godot_cpp/variant/utility_functions.hpp>
#include "voxel_world/entities/voxel_entity.h"

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
    cs = new ComputeShader(
        "res://addons/voxel_playground/src/shaders/voxel_renderer.glsl",
        _rd,
        {"#define TESTe", "#define ENABLE_ENTITIES 1", "#define DEBUG_ENTITY_RESULT_READ 1"}
    );

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

    //--------- ENTITY BUFFERS (discover and upload once) ---------
    discover_entities();
    // TEMP: Skip entity trace pass wiring for stability on Metal during bring-up.

    // Create persistent debug spheres for entities to visualize without entity traversal
    _entity_debug_spheres.clear();
    {
        SceneTree *tree = get_tree();
        if (tree) {
            Array nodes = tree->get_nodes_in_group("voxel_entities");
            for (int i = 0; i < nodes.size(); ++i) {
                VoxelEntity *e = Object::cast_to<VoxelEntity>(nodes[i]);
                if (!e) continue;
                // Compute world-space center and approximate radius
                Vector3i brick_map = e->get_brick_map_size();
                Vector3 grid = Vector3(brick_map.x * 8, brick_map.y * 8, brick_map.z * 8);
                float s = voxel_world ? voxel_world->get_scale() : 0.25f;
                Vector3 half_extents = 0.5f * s * grid;
                Vector3 center_local = half_extents; // entity local origin at (0,0,0)
                Vector3 center_world = e->get_global_transform().xform(center_local);
                float radius = half_extents.length();
                int pid = register_projectile(center_world, radius);
                _entity_debug_spheres.push_back(pid);
            }
        }
    }

    // Skip entity trace pass wiring for now.

    Ref<RDTextureView> output_texture_view = memnew(RDTextureView);
    { // output texture
        auto output_format = cs->create_texture_format(render_parameters.width, render_parameters.height, RenderingDevice::DATA_FORMAT_R32G32B32A32_SFLOAT);
        if (output_texture_rect == nullptr)
        {
            // Try to find a TextureRect child for convenience; fallback to headless rendering
            List<Node*> stack;
            get_tree()->get_nodes_in_group("voxel_camera_output");
            // DFS search for TextureRect child
            for (int i = 0; i < get_child_count(); ++i) stack.push_back(get_child(i));
            while (!stack.is_empty() && output_texture_rect == nullptr) {
                Node *n = stack.front()->get(); stack.pop_front();
                if (auto tr = Object::cast_to<TextureRect>(n)) { output_texture_rect = tr; break; }
                for (int j = 0; j < n->get_child_count(); ++j) stack.push_back(n->get_child(j));
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
            UtilityFunctions::print("VoxelCamera: output TextureRect assigned.");
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

    // render
    uint64_t raymarching_start = Time::get_singleton()->get_ticks_usec();
    Vector2i Size = {render_parameters.width, render_parameters.height};
    // Entity trace pass disabled for stability
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

void VoxelCamera::discover_entities()
{
    // Find VoxelEntity nodes
    _entities.clear();
    _all_entity_bricks.clear();
    _all_entity_voxels.clear();

    SceneTree *tree = get_tree();
    if (!tree) return;
    Array nodes = tree->get_nodes_in_group("voxel_entities");
    if (nodes.is_empty()) {
        // No entities to setup; still create tiny buffers to satisfy bindings
        PackedByteArray countbuf; countbuf.resize(sizeof(int));
        *reinterpret_cast<int*>(countbuf.ptrw()) = 0;
        entity_count_rid = cs->create_storage_buffer_uniform(countbuf, 6, 1);

        PackedByteArray descs; descs.resize(sizeof(EntityDescriptorCPU) * MAX_ENTITIES);
        descs.fill(0);
        entity_descriptors_rid = cs->create_storage_buffer_uniform(descs, 9, 1);

        PackedByteArray bricks; bricks.resize(sizeof(Brick)); bricks.fill(0);
        entity_bricks_rid = cs->create_storage_buffer_uniform(bricks, 7, 1);
        PackedByteArray vox; vox.resize(sizeof(Voxel)); vox.fill(0);
        entity_voxels_rid = cs->create_storage_buffer_uniform(vox, 8, 1);
        return;
    }

    // Build CPU entity volumes and descriptors
    int count = 0;
    for (int i = 0; i < nodes.size() && count < MAX_ENTITIES; ++i) {
        VoxelEntity *e = Object::cast_to<VoxelEntity>(nodes[i]);
        if (!e) continue;

        // Setup sizes
        Vector3i brick_map = e->get_brick_map_size();
        Vector3i grid = brick_map * Vector3i(8,8,8);
        Vector3i brick_grid = brick_map;
        int brick_count = brick_grid.x * brick_grid.y * brick_grid.z;

        // Generate voxels for a sphere slime (centered)
        std::vector<Voxel> local_voxels;
        local_voxels.resize(brick_count * VoxelWorldProperties::BRICK_VOLUME, Voxel::create_air_voxel());

        auto mortonIndex = [](int lx, int ly, int lz) {
            unsigned int morton = 0u;
            morton |= ((static_cast<unsigned int>(lx) >> 0) & 1u) << 0;
            morton |= ((static_cast<unsigned int>(ly) >> 0) & 1u) << 1;
            morton |= ((static_cast<unsigned int>(lz) >> 0) & 1u) << 2;
            morton |= ((static_cast<unsigned int>(lx) >> 1) & 1u) << 3;
            morton |= ((static_cast<unsigned int>(ly) >> 1) & 1u) << 4;
            morton |= ((static_cast<unsigned int>(lz) >> 1) & 1u) << 5;
            morton |= ((static_cast<unsigned int>(lx) >> 2) & 1u) << 6;
            morton |= ((static_cast<unsigned int>(ly) >> 2) & 1u) << 7;
            morton |= ((static_cast<unsigned int>(lz) >> 2) & 1u) << 8;
            return morton;
        };

        // Sphere parameters
        int r = e->get_radius_voxels();
        Vector3 center = Vector3(grid.x * 0.5f, grid.y * 0.5f, grid.z * 0.5f);
        Color col = e->get_color();
        Voxel solid = Voxel::create_solid_voxel(col);

        auto write_local_voxel = [&](int gx, int gy, int gz, Voxel v){
            // Compute brick index and index within brick (Morton)
            Vector3i brick_pos = Vector3i(gx/8, gy/8, gz/8);
            int brick_index = brick_pos.x + brick_pos.y * brick_grid.x + brick_pos.z * brick_grid.x * brick_grid.y;
            int lx = gx % 8, ly = gy % 8, lz = gz % 8;
            unsigned int in_brick = mortonIndex(lx, ly, lz);
            size_t base = size_t(brick_index) * VoxelWorldProperties::BRICK_VOLUME;
            local_voxels[base + in_brick] = v;
        };

        for (int z = 0; z < grid.z; ++z) {
            for (int y = 0; y < grid.y; ++y) {
                for (int x = 0; x < grid.x; ++x) {
                    Vector3 p = Vector3(x + 0.5f, y + 0.5f, z + 0.5f);
                    if ((p - center).length() <= float(r)) {
                        write_local_voxel(x, y, z, solid);
                    }
                }
            }
        }

        // Build bricks and occupancy
        std::vector<Brick> local_bricks;
        local_bricks.resize(brick_count);

        // Compute offsets into aggregated arrays
        uint32_t voxel_offset = static_cast<uint32_t>(_all_entity_voxels.size());
        uint32_t brick_offset = static_cast<uint32_t>(_all_entity_bricks.size());

        // Copy bricks + voxels into global arrays
        for (int bz = 0; bz < brick_grid.z; ++bz) {
            for (int by = 0; by < brick_grid.y; ++by) {
                for (int bx = 0; bx < brick_grid.x; ++bx) {
                    int bindex = bx + by * brick_grid.x + bz * brick_grid.x * brick_grid.y;
                    // Occupancy: count non-air in this brick
                    int count_non_air = 0;
                    size_t base = size_t(bindex) * VoxelWorldProperties::BRICK_VOLUME;
                    for (int i = 0; i < VoxelWorldProperties::BRICK_VOLUME; ++i) {
                        if (!local_voxels[base + i].is_air()) count_non_air++;
                    }
                    Brick b;
                    b.occupancy_count = count_non_air;
                    // pointer is in bricks; base brick = voxel_offset / BRICK_VOLUME
                    b.voxel_data_pointer = (voxel_offset / VoxelWorldProperties::BRICK_VOLUME) + bindex;
                    local_bricks[bindex] = b;
                }
            }
        }

        // Append to global arrays
        _all_entity_bricks.insert(_all_entity_bricks.end(), local_bricks.begin(), local_bricks.end());
        _all_entity_voxels.insert(_all_entity_voxels.end(), local_voxels.begin(), local_voxels.end());

        // Fill descriptor
        EntityDescriptorCPU desc = {};
        Transform3D xf = e->get_global_transform();
        // local origin at (0,0,0); local bounds in meters [0, grid*scale]
        Vector3 scale_v = Vector3(1,1,1); // handled by desc.scale
        Basis b = xf.get_basis();

        // local_to_world matrix (float[16])
        // Build affine as column-major 4x4
        Utils::transform_to_float(desc.local_to_world, xf);
        Transform3D wtol_t = xf.affine_inverse();
        Utils::transform_to_float(desc.world_to_local, wtol_t);

        // AABB in world space: corners of local box [0, grid*scale]
        float s = voxel_world ? voxel_world->get_scale() : 0.25f;
        Vector3 local_min(0,0,0);
        Vector3 local_max = Vector3(grid.x * s, grid.y * s, grid.z * s);
        Vector3 corners[8] = {
            Vector3(local_min.x, local_min.y, local_min.z),
            Vector3(local_max.x, local_min.y, local_min.z),
            Vector3(local_min.x, local_max.y, local_min.z),
            Vector3(local_min.x, local_min.y, local_max.z),
            Vector3(local_max.x, local_max.y, local_min.z),
            Vector3(local_max.x, local_min.y, local_max.z),
            Vector3(local_min.x, local_max.y, local_max.z),
            Vector3(local_max.x, local_max.y, local_max.z)
        };
        Vector3 wmin = xf.xform(corners[0]);
        Vector3 wmax = wmin;
        for (int c = 1; c < 8; ++c) {
            Vector3 w = xf.xform(corners[c]);
            wmin = wmin.min(w);
            wmax = wmax.max(w);
        }
        desc.aabb_min[0] = wmin.x; desc.aabb_min[1] = wmin.y; desc.aabb_min[2] = wmin.z; desc.aabb_min[3] = 0.0f;
        desc.aabb_max[0] = wmax.x; desc.aabb_max[1] = wmax.y; desc.aabb_max[2] = wmax.z; desc.aabb_max[3] = 0.0f;
        desc.grid_size[0] = (float)grid.x; desc.grid_size[1] = (float)grid.y; desc.grid_size[2] = (float)grid.z; desc.grid_size[3] = 0.0f;
        desc.brick_grid_size[0] = (float)brick_grid.x; desc.brick_grid_size[1] = (float)brick_grid.y; desc.brick_grid_size[2] = (float)brick_grid.z; desc.brick_grid_size[3] = 0.0f;
        desc.scale = s;
        desc.brick_offset = brick_offset;
        desc.voxel_offset = voxel_offset;
        desc.brick_count = brick_count;
        desc.enabled = 1;
        desc.health = e->get_health();

        _entities.push_back(desc);
        count++;
    }

    // Create GPU buffers with aggregated data
    PackedByteArray countbuf; countbuf.resize(sizeof(int));
    // Enable entities for AABB debug: upload true count
    int upload_count = count;
    *reinterpret_cast<int*>(countbuf.ptrw()) = upload_count;
    entity_count_rid = cs->create_storage_buffer_uniform(countbuf, 6, 1);

    PackedByteArray descs; descs.resize(sizeof(EntityDescriptorCPU) * MAX_ENTITIES);
    descs.fill(0);
    if (count > 0) {
        std::memcpy(descs.ptrw(), _entities.ptr(), sizeof(EntityDescriptorCPU) * _entities.size());
    }
    entity_descriptors_rid = cs->create_storage_buffer_uniform(descs, 9, 1);

    // Bricks
    PackedByteArray bricks;
    bricks.resize(_all_entity_bricks.size() * sizeof(Brick));
    if (!bricks.is_empty()) {
        std::memcpy(bricks.ptrw(), _all_entity_bricks.data(), _all_entity_bricks.size() * sizeof(Brick));
    }
    entity_bricks_rid = cs->create_storage_buffer_uniform(bricks, 7, 1);

    // Voxels
    PackedByteArray vox;
    vox.resize(_all_entity_voxels.size() * sizeof(Voxel));
    if (!vox.is_empty()) {
        std::memcpy(vox.ptrw(), _all_entity_voxels.data(), _all_entity_voxels.size() * sizeof(Voxel));
    }
    entity_voxels_rid = cs->create_storage_buffer_uniform(vox, 8, 1);

    UtilityFunctions::print("VoxelCamera: discovered ", count, " voxel entities. desc size=", (int)sizeof(EntityDescriptorCPU));
    if (count > 0) {
        const EntityDescriptorCPU &d = _entities[0];
        UtilityFunctions::print("Entity0 aabb_min= (", d.aabb_min[0], ",", d.aabb_min[1], ",", d.aabb_min[2], ") aabb_max= (", d.aabb_max[0], ",", d.aabb_max[1], ",", d.aabb_max[2], ") grid= (", d.grid_size[0], ",", d.grid_size[1], ",", d.grid_size[2], ") bricks= (", d.brick_grid_size[0], ",", d.brick_grid_size[1], ",", d.brick_grid_size[2], ") scale= ", d.scale, ", brick_offset= ", (int)d.brick_offset, ", voxel_offset= ", (int)d.voxel_offset, ", brick_count= ", (int)d.brick_count);
        UtilityFunctions::print("DEBUG: Uploading entity_count=", upload_count);
    }
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
