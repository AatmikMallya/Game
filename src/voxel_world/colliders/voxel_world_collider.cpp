#include "voxel_world_collider.h"

void VoxelWorldCollider::_bind_methods()
{
    ClassDB::bind_method(D_METHOD("_on_data_fetched", "data"), &VoxelWorldCollider::onDataFetched);

    ClassDB::bind_method(D_METHOD("get_collider_size"), &VoxelWorldCollider::getColliderSize);
    ClassDB::bind_method(D_METHOD("set_collider_size", "size"), &VoxelWorldCollider::setColliderSize);
    ADD_PROPERTY(PropertyInfo(Variant::VECTOR3I, "collider_size"), "set_collider_size", "get_collider_size");

    ClassDB::bind_method(D_METHOD("get_collision_shape"), &VoxelWorldCollider::getCollisionShape);
    ClassDB::bind_method(D_METHOD("set_collision_shape", "shape"), &VoxelWorldCollider::setCollisionShape);
    ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "collision_shape", PROPERTY_HINT_NODE_TYPE, "CollisionShape3D"),
                 "set_collision_shape", "get_collision_shape");


    ClassDB::bind_method(D_METHOD("get_update_interval"), &VoxelWorldCollider::getUpdateInterval);
    ClassDB::bind_method(D_METHOD("set_update_interval", "interval"), &VoxelWorldCollider::setUpdateInterval);
    ADD_PROPERTY(PropertyInfo(Variant::INT, "update_interval", PROPERTY_HINT_RANGE, "1,60,1"),
                 "set_update_interval", "get_update_interval");
}

void VoxelWorldCollider::init(RenderingDevice *rd, VoxelWorldRIDs& voxel_world_rids, float scale)
{
    this->scale = scale;
    if (_collision_shape == nullptr)
    {
        UtilityFunctions::printerr("Collider shape is null, cannot initialize VoxelWorldCollider");
        return;
    }

    // init collision
    _collision_polygon.instantiate();
    _collision_shape->set_shape(_collision_polygon);

    // init shader
    _collider_params = {
        Vector4i(0, 0, 0, 1),                                              // min corner
        Vector4i(_collider_size.x, _collider_size.y, _collider_size.z, 0), // size
    };

    // Number of voxels
    const int64_t nvox = int64_t(_collider_size.x) * _collider_size.y * _collider_size.z;

    // Storage as 32-bit words (1 bit per voxel)
    const int64_t uint_count = (nvox + 31) / 32;            // ceil(nvox / 32)
    const int64_t byte_size  = uint_count * sizeof(uint32_t);

    PackedByteArray collider_voxel_data;
    collider_voxel_data.resize((int)byte_size);
    collider_voxel_data.fill(0);

    _fetch_data_shader =
        new ComputeShader("res://addons/voxel_playground/src/shaders/voxel_edit/fetch_solid_mask.glsl", rd);
        voxel_world_rids.add_voxel_buffers(_fetch_data_shader);
    _collider_params_rid =
        _fetch_data_shader->create_storage_buffer_uniform(_collider_params.to_packed_byte_array(), 0, 1);
    _collider_voxel_data_rid = _fetch_data_shader->create_storage_buffer_uniform(collider_voxel_data, 1, 1);
    _fetch_data_shader->finish_create_uniforms();

    _is_updating = false;
    frames_since_last_update = 0;
}

void VoxelWorldCollider::addQuad(const Vector3 &p1, const int dir, const bool flip)
{
    const static Vector3 offsets[3][4] = {{// YZ plane
                                           Vector3(0, 0, 0), Vector3(0, 1, 0), Vector3(0, 1, 1), Vector3(0, 0, 1)},
                                          {// XZ plane
                                           Vector3(0, 0, 0), Vector3(0, 0, 1), Vector3(1, 0, 1), Vector3(1, 0, 0)},
                                          {// XY plane
                                           Vector3(0, 0, 0), Vector3(1, 0, 0), Vector3(1, 1, 0), Vector3(0, 1, 0)}};
    // add triangle
    Vector3 v0 = p1;
    Vector3 v1 = p1 + offsets[dir][1] * scale;
    Vector3 v2 = p1 + offsets[dir][2] * scale;
    Vector3 v3 = p1 + offsets[dir][3] * scale;

    if (flip)
    {
        _collider_vertices.push_back(v0);
        _collider_vertices.push_back(v1);
        _collider_vertices.push_back(v2);

        _collider_vertices.push_back(v0);
        _collider_vertices.push_back(v2);
        _collider_vertices.push_back(v3);
    }
    else
    {
        _collider_vertices.push_back(v0);
        _collider_vertices.push_back(v2);
        _collider_vertices.push_back(v1);

        _collider_vertices.push_back(v0);
        _collider_vertices.push_back(v3);
        _collider_vertices.push_back(v2);
    }
}

void VoxelWorldCollider::onDataFetched(const PackedByteArray &data)
{
    if (data.size() % sizeof(unsigned int) != 0)
    {
        UtilityFunctions::printerr("Data size is not a multiple of 4 bytes.");
        return;
    }

    const size_t num_uints = data.size() / sizeof(unsigned int);
    _collider_voxel_data.resize(num_uints);
    memcpy(_collider_voxel_data.data(), data.ptr(), data.size());

    _collider_vertices.clear();
    for (size_t x = 0; x < _collider_size.x; x++)
    {
        for (size_t y = 0; y < _collider_size.y; y++)
        {
            for (size_t z = 0; z < _collider_size.z; z++)
            {
                Vector3i ipos(x, y, z);
                Vector3 pos = scale * Vector3(x, y, z) + collider_offset;
                bool value = is_voxel_air(ipos);
                if (x > 0 && (value ^ is_voxel_air(Vector3i(x - 1, y, z))))
                {
                    addQuad(pos, 0, value);
                }
                if (y > 0 && (value ^ is_voxel_air(Vector3i(x, y - 1, z))))
                {
                    addQuad(pos, 1, value);
                }
                if (z > 0 && (value ^ is_voxel_air(Vector3i(x, y, z - 1))))
                {
                    addQuad(pos, 2, value);
                }
            }
        }
    }

    _collision_polygon->set_faces(_collider_vertices);

    _is_updating = false;
}

void VoxelWorldCollider::update(Vector3i position)
{
    if (_fetch_data_shader == nullptr || !_fetch_data_shader->check_ready())
    {
        UtilityFunctions::printerr("Collider fetch voxel data shader is null or not ready");
        return;
    }

    frames_since_last_update--;
    if (_is_updating || frames_since_last_update > 0)
        return;
    _is_updating = true;
    frames_since_last_update = _update_interval;

    position -= (_collider_size / 2); // set it to the min corner
    collider_offset = Vector3(position.x, position.y, position.z) * scale;
    _collider_params.bounds_min = Vector4i(position.x, position.y, position.z, 0);

    _fetch_data_shader->update_storage_buffer_uniform(_collider_params_rid, _collider_params.to_packed_byte_array());

    const Vector3 group_size = Vector3(8, 8, 8);
    const Vector3i group_count = Vector3i(std::ceil(_collider_size.x / group_size.x),
                                          std::ceil(_collider_size.y / group_size.y),
                                          std::ceil(_collider_size.z / group_size.z));

    _fetch_data_shader->compute(group_count, false);

    _fetch_data_shader->get_storage_buffer_uniform_async(_collider_voxel_data_rid, Callable(this, "_on_data_fetched"));

    // PackedByteArray arr =  ray_cast_shader->get_storage_buffer_uniform(_edit_params_rid);
    // VoxelEditParams *params = reinterpret_cast<VoxelEditParams *>(arr.ptrw());
}

bool VoxelWorldCollider::is_voxel_air(Vector3i pos)
{
    unsigned int index = pos.x + pos.y * _collider_params.bounds_size.x +
                         pos.z * _collider_params.bounds_size.x * _collider_params.bounds_size.y;
    unsigned int bit_index = index % 32;
    unsigned int data_index = index / 32;
    
    if (data_index >= _collider_voxel_data.size()) return true;
    return ((_collider_voxel_data[data_index] & (1u << bit_index)) != 0);
}
