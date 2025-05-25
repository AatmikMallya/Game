#ifndef VOXEL_WORLD_H
#define VOXEL_WORLD_H

#include <godot_cpp/classes/node3d.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/vector3i.hpp>
#include <godot_cpp/classes/rendering_device.hpp>

#include "voxel_world/properties.h"
#include "voxel_world/cellular_automata/voxel_world_update_pass.h"

using namespace godot;

class VoxelWorld : public Node3D {
    GDCLASS(VoxelWorld, Node3D);

protected:
    static void _bind_methods();
    void _notification(int what);

private:
    // The size property will store the dimensions of your voxel world in terms of brick counts.
    const Vector3i BRICK_SIZE = Vector3i(8,8,8);
    Vector3i brick_map_size = Vector3i(16, 16, 16);
    float scale = 0.125f;
    bool simulation_enabled = true;

    RID _voxel_data_rid;
    RID _voxel_bricks_rid;
    RID _voxel_properties_rid;
    VoxelWorldProperties _voxel_properties;
    RenderingDevice* _rd;

    VoxelWorldUpdatePass* _update_pass = nullptr;

    void init();
    void update(float delta);

public:
    VoxelWorld();
    ~VoxelWorld();    

    // Property accessors for size.
    void set_brick_map_size(const Vector3i &p_size) { brick_map_size = p_size; }
    Vector3i get_brick_map_size() const { return brick_map_size; }

    void set_scale(float p_scale) { scale = p_scale; }
    float get_scale() const { return scale; }

    void set_simulation_enabled(bool enabled) { simulation_enabled = enabled; }
    bool get_simulation_enabled() const { return simulation_enabled; }

    RID get_voxel_data_rid() const { return _voxel_data_rid; }
    RID get_voxel_bricks_rid() const { return _voxel_bricks_rid; }
    RID get_voxel_properties_rid() const { return _voxel_properties_rid; }
    VoxelWorldProperties get_voxel_properties() const { return _voxel_properties; }
    
};

#endif // VOXEL_WORLD_H
