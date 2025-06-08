#ifndef VOXEL_WORLD_COLLIDER_H
#define VOXEL_WORLD_COLLIDER_H

#include <godot_cpp/classes/node3d.hpp>
#include <godot_cpp/classes/rendering_device.hpp>
#include <godot_cpp/classes/collision_shape3d.hpp>
#include <godot_cpp/classes/concave_polygon_shape3d.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/rid.hpp>

#include "gdcs/include/gdcs.h"
#include "voxel_world/voxel_properties.h"

using namespace godot;

class VoxelWorldCollider : public Node3D
{
    GDCLASS(VoxelWorldCollider, Node3D);

    struct VoxelColliderParams
    {
        Vector4i bounds_min;
        Vector4i bounds_size;

        PackedByteArray to_packed_byte_array()
        {
            PackedByteArray byte_array;
            byte_array.resize(sizeof(VoxelColliderParams));
            std::memcpy(byte_array.ptrw(), this, sizeof(VoxelColliderParams));
            return byte_array;
        }
    };

  protected:
    static void _bind_methods();

  public:
    void init(RenderingDevice *rd, VoxelWorldRIDs& voxel_world_rids, float scale);
    void addQuad(const Vector3 &p1, const int dir, const bool flip);
    void onDataFetched(const PackedByteArray &data);
    VoxelWorldCollider() {};
    ~VoxelWorldCollider() {};

    void update(Vector3i position);

    Vector3i getColliderSize() const { return _collider_size;}
    void setColliderSize(const Vector3i &size) { _collider_size = size; }

    int getUpdateInterval() const { return _update_interval;}
    void setUpdateInterval(const int interval) { _update_interval = interval; }

    CollisionShape3D *getCollisionShape() const { return _collision_shape; }
    void setCollisionShape(CollisionShape3D *shape) { _collision_shape = shape; }

  private:
    bool isVoxelAir(Vector3i pos);

    int frames_since_last_update = 0;
    int _update_interval = 15;
    bool _is_updating = false;

    Vector3i _collider_size;

    Vector3 collider_offset = Vector3(0, 0, 0);
    float scale = 0.125f;

    CollisionShape3D *_collision_shape = nullptr;
    Ref<ConcavePolygonShape3D> _collision_polygon = nullptr;
    ComputeShader *_fetch_data_shader = nullptr;

    VoxelColliderParams _collider_params;
    std::vector<unsigned int> _collider_voxel_data;
    PackedVector3Array _collider_vertices;

    RID _collider_params_rid;
    RID _collider_voxel_data_rid;
};

#endif // VOXEL_WORLD_COLLIDER_H
