#ifndef WAVE_FUNCTION_COLLAPSE_TILE_H
#define WAVE_FUNCTION_COLLAPSE_TILE_H

#include "voxel_world/generator/voxel_world_generator.h"
#include <godot_cpp/classes/node3d.hpp>
#include <godot_cpp/classes/rendering_device.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/rid.hpp>

#include "voxel_world/data/voxel_data_vox.h"

using namespace godot;

class WaveFunctionCollapseTile : public Resource {
    GDCLASS(WaveFunctionCollapseTile, Resource)

public:
    void set_voxel_tile(const Ref<VoxelData> &tile) { voxel_tile = tile; }
    Ref<VoxelData> get_voxel_tile() const { return voxel_tile; }

    void set_weight(float w) { weight = w; }
    float get_weight() const { return weight; }

    void set_can_rotate_y(bool can_rotate) { can_rotate_y = can_rotate; }
    bool get_can_rotate_y() const { return can_rotate_y; }

    void set_can_flip_horizontal(bool can_flip_horizontal) { this->can_flip_horizontal = can_flip_horizontal; }
    bool get_can_flip_horizontal() const { return can_flip_horizontal; }

    static void _bind_methods() {
        ClassDB::bind_method(D_METHOD("set_voxel_tile", "tile"), &WaveFunctionCollapseTile::set_voxel_tile);
        ClassDB::bind_method(D_METHOD("get_voxel_tile"), &WaveFunctionCollapseTile::get_voxel_tile);
        ClassDB::bind_method(D_METHOD("set_weight", "weight"), &WaveFunctionCollapseTile::set_weight);
        ClassDB::bind_method(D_METHOD("get_weight"), &WaveFunctionCollapseTile::get_weight);
        ClassDB::bind_method(D_METHOD("set_can_rotate_y", "can_rotate"), &WaveFunctionCollapseTile::set_can_rotate_y);
        ClassDB::bind_method(D_METHOD("get_can_rotate_y"), &WaveFunctionCollapseTile::get_can_rotate_y);
        ClassDB::bind_method(D_METHOD("set_can_flip_horizontal", "can_flip_horizontal"), &WaveFunctionCollapseTile::set_can_flip_horizontal);
        ClassDB::bind_method(D_METHOD("get_can_flip_horizontal"), &WaveFunctionCollapseTile::get_can_flip_horizontal);

        ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "voxel_tile", PROPERTY_HINT_RESOURCE_TYPE, "VoxelData"), "set_voxel_tile", "get_voxel_tile");
        ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "weight"), "set_weight", "get_weight");
        ADD_PROPERTY(PropertyInfo(Variant::BOOL, "can_rotate_y"), "set_can_rotate_y", "get_can_rotate_y");
        ADD_PROPERTY(PropertyInfo(Variant::BOOL, "can_flip_horizontal"), "set_can_flip_horizontal", "get_can_flip_horizontal");
    }

protected:

    Ref<VoxelData> voxel_tile;
    float weight = 1.0f;
    bool can_rotate_y = true;
    bool can_flip_horizontal = true;
};

#endif // WAVE_FUNCTION_TILE
