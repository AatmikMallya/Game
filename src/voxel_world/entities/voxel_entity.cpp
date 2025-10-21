#include "voxel_entity.h"

void VoxelEntity::_bind_methods()
{
    ClassDB::bind_method(D_METHOD("set_brick_map_size", "size"), &VoxelEntity::set_brick_map_size);
    ClassDB::bind_method(D_METHOD("get_brick_map_size"), &VoxelEntity::get_brick_map_size);
    ADD_PROPERTY(PropertyInfo(Variant::VECTOR3I, "brick_map_size"), "set_brick_map_size", "get_brick_map_size");

    ClassDB::bind_method(D_METHOD("set_radius_voxels", "radius"), &VoxelEntity::set_radius_voxels);
    ClassDB::bind_method(D_METHOD("get_radius_voxels"), &VoxelEntity::get_radius_voxels);
    ADD_PROPERTY(PropertyInfo(Variant::INT, "radius_voxels", PROPERTY_HINT_RANGE, "1,64,1"), "set_radius_voxels", "get_radius_voxels");

    ClassDB::bind_method(D_METHOD("set_color", "color"), &VoxelEntity::set_color);
    ClassDB::bind_method(D_METHOD("get_color"), &VoxelEntity::get_color);
    ADD_PROPERTY(PropertyInfo(Variant::COLOR, "color"), "set_color", "get_color");

    ClassDB::bind_method(D_METHOD("set_health", "value"), &VoxelEntity::set_health);
    ClassDB::bind_method(D_METHOD("get_health"), &VoxelEntity::get_health);
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "health"), "set_health", "get_health");
}

void VoxelEntity::_notification(int what)
{
    switch (what) {
        case NOTIFICATION_READY: {
            add_to_group("voxel_entities");
            break;
        }
        default: break;
    }
}

