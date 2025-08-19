#ifndef VOXEL_WORLD_WFC_TILE_GENERATOR_H
#define VOXEL_WORLD_WFC_TILE_GENERATOR_H

#include "voxel_world/generator/voxel_world_generator.h"
#include <godot_cpp/classes/node3d.hpp>
#include <godot_cpp/classes/rendering_device.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/array.hpp>
#include <godot_cpp/variant/rid.hpp>

#include "voxel_world/data/voxel_data_vox.h"
#include "voxel_world_wfc_tile.h"
#include "wave_function_collapse_generator.h"

using namespace godot;

class VoxelWorldWFCTileGenerator : public WaveFunctionCollapseGenerator
{
    GDCLASS(VoxelWorldWFCTileGenerator, WaveFunctionCollapseGenerator)

  public:
    // --- Existing voxel_tiles property ---
    TypedArray<WaveFunctionCollapseTile> get_voxel_tiles() const { return voxel_tiles; }
    void set_voxel_tiles(const TypedArray<WaveFunctionCollapseTile> &tiles) { voxel_tiles = tiles; }

    // --- New seed position property ---
    Vector3 get_seed_position_normalized() const { return seed_position_normalized; }
    void set_seed_position_normalized(const Vector3 &pos) {
        // Clamp to [0, 1] per component
        seed_position_normalized.x = Math::clamp(pos.x, 0.0f, 1.0f);
        seed_position_normalized.y = Math::clamp(pos.y, 0.0f, 1.0f);
        seed_position_normalized.z = Math::clamp(pos.z, 0.0f, 1.0f);
    }

    // --- New boolean properties ---
    bool get_need_nonair_connection() const { return need_nonair_connection; }
    void set_need_nonair_connection(bool value) { need_nonair_connection = value; }

    bool get_need_air_connection_edge() const { return need_air_connection_edge; }
    void set_need_air_connection_edge(bool value) { need_air_connection_edge = value; }

    bool get_show_contradictions() const { return show_contradictions; }
    void set_show_contradictions(bool value) { show_contradictions = value; }

    bool get_only_replace_air() const { return only_replace_air; }
    void set_only_replace_air(bool value) { only_replace_air = value; }

    bool prepare_voxel_tiles(const TypedArray<WaveFunctionCollapseTile> &tiles);

    bool generate(std::vector<Voxel> &result_voxels,
                  const Vector3i bounds_min,
                  const Vector3i bounds_max,
                  const VoxelWorldProperties &properties) override;

    static void _bind_methods()
    {
        // voxel_tiles
        ClassDB::bind_method(D_METHOD("get_voxel_tiles"), &VoxelWorldWFCTileGenerator::get_voxel_tiles);
        ClassDB::bind_method(D_METHOD("set_voxel_tiles", "value"), &VoxelWorldWFCTileGenerator::set_voxel_tiles);
        ADD_PROPERTY(PropertyInfo(Variant::ARRAY, "voxel_tiles", PROPERTY_HINT_TYPE_STRING,
                                  String::num(Variant::OBJECT) + "/" +
                                  String::num(PROPERTY_HINT_RESOURCE_TYPE) + ":WaveFunctionCollapseTile"),
                     "set_voxel_tiles", "get_voxel_tiles");

        // seed_position_normalized
        ClassDB::bind_method(D_METHOD("get_seed_position_normalized"), &VoxelWorldWFCTileGenerator::get_seed_position_normalized);
        ClassDB::bind_method(D_METHOD("set_seed_position_normalized", "pos"), &VoxelWorldWFCTileGenerator::set_seed_position_normalized);
        ADD_PROPERTY(PropertyInfo(Variant::VECTOR3, "seed_position_normalized",
                                  PROPERTY_HINT_RANGE, "0,1,0.001"),
                     "set_seed_position_normalized", "get_seed_position_normalized");

        // need_nonair_connection
        ClassDB::bind_method(D_METHOD("get_need_nonair_connection"), &VoxelWorldWFCTileGenerator::get_need_nonair_connection);
        ClassDB::bind_method(D_METHOD("set_need_nonair_connection", "value"), &VoxelWorldWFCTileGenerator::set_need_nonair_connection);
        ADD_PROPERTY(PropertyInfo(Variant::BOOL, "need_nonair_connection"),
                     "set_need_nonair_connection", "get_need_nonair_connection");

        // need_air_connection_edge
        ClassDB::bind_method(D_METHOD("get_need_air_connection_edge"), &VoxelWorldWFCTileGenerator::get_need_air_connection_edge);
        ClassDB::bind_method(D_METHOD("set_need_air_connection_edge", "value"), &VoxelWorldWFCTileGenerator::set_need_air_connection_edge);
        ADD_PROPERTY(PropertyInfo(Variant::BOOL, "need_air_connection_edge"),
                     "set_need_air_connection_edge", "get_need_air_connection_edge");

        // show_contradictions
        ClassDB::bind_method(D_METHOD("get_show_contradictions"), &VoxelWorldWFCTileGenerator::get_show_contradictions);
        ClassDB::bind_method(D_METHOD("set_show_contradictions", "value"), &VoxelWorldWFCTileGenerator::set_show_contradictions);
        ADD_PROPERTY(PropertyInfo(Variant::BOOL, "show_contradictions"),
                     "set_show_contradictions", "get_show_contradictions");

        // only_replace_air
        ClassDB::bind_method(D_METHOD("get_only_replace_air"), &VoxelWorldWFCTileGenerator::get_only_replace_air);
        ClassDB::bind_method(D_METHOD("set_only_replace_air", "value"), &VoxelWorldWFCTileGenerator::set_only_replace_air);
        ADD_PROPERTY(PropertyInfo(Variant::BOOL, "only_replace_air"),
                     "set_only_replace_air", "get_only_replace_air");
    }

  protected:
    TypedArray<WaveFunctionCollapseTile> voxel_tiles;
    Vector3 seed_position_normalized = Vector3(0.5, 0.5, 0.5);
    bool need_nonair_connection = true;
    bool need_air_connection_edge = true;
    bool show_contradictions = true;
    bool only_replace_air = true;
};

#endif // VOXEL_WORLD_WFC_TILE_GENERATOR_H
