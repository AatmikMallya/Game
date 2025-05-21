#ifndef VOXEL_WORLD_H
#define VOXEL_WORLD_H

#include <godot_cpp/classes/node3d.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/vector3i.hpp>
#include <godot_cpp/classes/rendering_device.hpp>

#include "voxel_world/properties.h"

using namespace godot;

class VoxelWorld : public Node3D {
    GDCLASS(VoxelWorld, Node3D);

protected:
    static void _bind_methods();
    void _notification(int what);

private:
    // The size property will store the dimensions of your voxel world in terms of brick counts.
    Vector3i size;
    float scale = 0.1f;

    RID _voxel_data_rid;
    RID _voxel_properties_rid;
    VoxelWorldProperties _voxel_properties;
    RenderingDevice* _rd;

    void init();
    void update(float delta);

public:
    VoxelWorld();
    ~VoxelWorld();    

    // Property accessors for size.
    void set_size(const Vector3i &p_size) { size = p_size; }
    Vector3i get_size() const { return size; }

    void set_scale(float p_scale) { scale = p_scale; }
    float get_scale() const { return scale; }

    RID get_voxel_data_rid() const { return _voxel_data_rid; }
    RID get_voxel_properties_rid() const { return _voxel_properties_rid; }
    VoxelWorldProperties get_voxel_properties() const { return _voxel_properties; }
    
};

#endif // VOXEL_WORLD_H
