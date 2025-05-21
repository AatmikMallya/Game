#include "register_types.h"
#include "voxel_rendering/voxel_camera.h"
#include "voxel_world/voxel_world.h"

using namespace godot;

void initialize_voxel_playground_module(ModuleInitializationLevel p_level)
{
    if (p_level == MODULE_INITIALIZATION_LEVEL_SCENE)
    {
        GDREGISTER_CLASS(VoxelCamera);
        GDREGISTER_CLASS(VoxelWorld);
    }
}

void uninitialize_voxel_playground_module(ModuleInitializationLevel p_level)
{
    if (p_level == MODULE_INITIALIZATION_LEVEL_SCENE)
    {
    }
}

extern "C"
{
    // Initialization.
    GDExtensionBool GDE_EXPORT voxel_playground_library_init(GDExtensionInterfaceGetProcAddress p_get_proc_address,
                                                              const GDExtensionClassLibraryPtr p_library,
                                                              GDExtensionInitialization *r_initialization)
    {
        godot::GDExtensionBinding::InitObject init_obj(p_get_proc_address, p_library, r_initialization);

        init_obj.register_initializer(initialize_voxel_playground_module);
        init_obj.register_terminator(uninitialize_voxel_playground_module);
        init_obj.set_minimum_library_initialization_level(MODULE_INITIALIZATION_LEVEL_SCENE);

        return init_obj.init();
    }
}