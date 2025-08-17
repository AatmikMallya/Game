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
    TypedArray<WaveFunctionCollapseTile> get_voxel_tiles() const
    {
        return voxel_tiles;
    }
    void set_voxel_tiles(const TypedArray<WaveFunctionCollapseTile> &tiles)
    {
        voxel_tiles = tiles;
    }

    bool prepare_voxel_tiles(const TypedArray<WaveFunctionCollapseTile> &tiles);

    void generate(RenderingDevice *rd, VoxelWorldRIDs &voxel_world_rids,
                  const VoxelWorldProperties &properties) override;

    static void _bind_methods()
    {
        ClassDB::bind_method(D_METHOD("get_voxel_tiles"), &VoxelWorldWFCTileGenerator::get_voxel_tiles);
        ClassDB::bind_method(D_METHOD("set_voxel_tiles", "value"), &VoxelWorldWFCTileGenerator::set_voxel_tiles);
        ADD_PROPERTY(PropertyInfo(Variant::ARRAY, "voxel_tiles", PROPERTY_HINT_TYPE_STRING,
                                  String::num(Variant::OBJECT) + "/" + String::num(PROPERTY_HINT_RESOURCE_TYPE) +
                                      ":WaveFunctionCollapseTile"),
                     "set_voxel_tiles", "get_voxel_tiles");
    }

  protected:
    TypedArray<WaveFunctionCollapseTile> voxel_tiles;
};

#endif // VOXEL_WORLD_WFC_TILE_GENERATOR_H
