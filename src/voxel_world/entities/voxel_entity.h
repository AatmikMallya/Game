#ifndef VOXEL_ENTITY_H
#define VOXEL_ENTITY_H

#include <godot_cpp/classes/node3d.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/vector3i.hpp>
#include <godot_cpp/variant/color.hpp>

using namespace godot;

class VoxelEntity : public Node3D {
    GDCLASS(VoxelEntity, Node3D);

  protected:
    static void _bind_methods();
    void _notification(int what);

  public:
    VoxelEntity() = default;
    ~VoxelEntity() = default;

    void set_brick_map_size(const Vector3i &p) { brick_map_size = p; }
    Vector3i get_brick_map_size() const { return brick_map_size; }

    void set_radius_voxels(int r) { radius_voxels = r; }
    int get_radius_voxels() const { return radius_voxels; }

    void set_color(const Color &c) { color = c; }
    Color get_color() const { return color; }

    void set_health(float h) { health = h; }
    float get_health() const { return health; }

  private:
    Vector3i brick_map_size = Vector3i(3, 3, 3); // ~24^3 voxels
    int radius_voxels = 10;
    Color color = Color(0.2f, 0.9f, 0.45f, 1.0f); // green-ish
    float health = 50.0f;
};

#endif // VOXEL_ENTITY_H

