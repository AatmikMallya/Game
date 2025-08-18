#include "voxel_world_terrain_generator.h"
#include <algorithm>
#include <godot_cpp/classes/fast_noise_lite.hpp>
#include <godot_cpp/variant/utility_functions.hpp>

void VoxelWorldTerrainGenerator::_bind_methods()
{
    ClassDB::bind_method(D_METHOD("set_noise_frequency", "freq"), &VoxelWorldTerrainGenerator::set_noise_frequency);
    ClassDB::bind_method(D_METHOD("get_noise_frequency"), &VoxelWorldTerrainGenerator::get_noise_frequency);
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "noise_frequency"), "set_noise_frequency", "get_noise_frequency");

    ClassDB::bind_method(D_METHOD("set_noise_octaves", "oct"), &VoxelWorldTerrainGenerator::set_noise_octaves);
    ClassDB::bind_method(D_METHOD("get_noise_octaves"), &VoxelWorldTerrainGenerator::get_noise_octaves);
    ADD_PROPERTY(PropertyInfo(Variant::INT, "noise_octaves"), "set_noise_octaves", "get_noise_octaves");

    ClassDB::bind_method(D_METHOD("set_base_height", "h"), &VoxelWorldTerrainGenerator::set_base_height);
    ClassDB::bind_method(D_METHOD("get_base_height"), &VoxelWorldTerrainGenerator::get_base_height);
    ADD_PROPERTY(PropertyInfo(Variant::INT, "base_height"), "set_base_height", "get_base_height");

    ClassDB::bind_method(D_METHOD("set_height_variation", "var"), &VoxelWorldTerrainGenerator::set_height_variation);
    ClassDB::bind_method(D_METHOD("get_height_variation"), &VoxelWorldTerrainGenerator::get_height_variation);
    ADD_PROPERTY(PropertyInfo(Variant::INT, "height_variation"), "set_height_variation", "get_height_variation");

    ClassDB::bind_method(D_METHOD("set_flat_plot_size", "size"), &VoxelWorldTerrainGenerator::set_flat_plot_size);
    ClassDB::bind_method(D_METHOD("get_flat_plot_size"), &VoxelWorldTerrainGenerator::get_flat_plot_size);
    ADD_PROPERTY(PropertyInfo(Variant::VECTOR3I, "flat_plot_size"), "set_flat_plot_size", "get_flat_plot_size");

    ClassDB::bind_method(D_METHOD("set_plot_center", "center"), &VoxelWorldTerrainGenerator::set_plot_center);
    ClassDB::bind_method(D_METHOD("get_plot_center"), &VoxelWorldTerrainGenerator::get_plot_center);
    ADD_PROPERTY(PropertyInfo(Variant::VECTOR3I, "plot_center"), "set_plot_center", "get_plot_center");

    ClassDB::bind_method(D_METHOD("set_ground_color", "color"), &VoxelWorldTerrainGenerator::set_ground_color);
    ClassDB::bind_method(D_METHOD("get_ground_color"), &VoxelWorldTerrainGenerator::get_ground_color);
    ADD_PROPERTY(PropertyInfo(Variant::COLOR, "ground_color"), "set_ground_color", "get_ground_color");

    ClassDB::bind_method(D_METHOD("set_grass_color", "color"), &VoxelWorldTerrainGenerator::set_grass_color);
    ClassDB::bind_method(D_METHOD("get_grass_color"), &VoxelWorldTerrainGenerator::get_grass_color);
    ADD_PROPERTY(PropertyInfo(Variant::COLOR, "grass_color"), "set_grass_color", "get_grass_color");

    ClassDB::bind_method(D_METHOD("set_structure_pass", "structure_pass"),
                         &VoxelWorldTerrainGenerator::set_structure_pass);
    ClassDB::bind_method(D_METHOD("get_structure_pass"), &VoxelWorldTerrainGenerator::get_structure_pass);
    ADD_PROPERTY(
        PropertyInfo(Variant::OBJECT, "structure_pass", PROPERTY_HINT_RESOURCE_TYPE, "VoxelWorldGeneratorCPUPass"),
        "set_structure_pass", "get_structure_pass");
}
bool VoxelWorldTerrainGenerator::generate(std::vector<Voxel> &voxel_data, const Vector3i bounds_min,
                                          const Vector3i bounds_max, const VoxelWorldProperties &properties)
{
    Vector3i volume_size = bounds_max - bounds_min;
    // Prepare voxel array
    voxel_data.assign((size_t)volume_size.x * volume_size.y * volume_size.z, Voxel::create_air_voxel());

    // Configure noise
    Ref<FastNoiseLite> noise;
    noise.instantiate();
    noise->set_noise_type(FastNoiseLite::TYPE_PERLIN);
    noise->set_frequency(noise_frequency);
    noise->set_fractal_octaves(noise_octaves);

    auto index3d = [&](int x, int y, int z) {
        return ((size_t)z * volume_size.y + (size_t)y) * volume_size.x + (size_t)x;
    };

    // Precompute plot bounds
    Vector3i plot_buffer_vec = Vector3i(plot_buffer, plot_buffer, plot_buffer);
    Vector3i half_plot = flat_plot_size / 2;
    Vector3i plot_min = plot_center - half_plot;
    Vector3i plot_max = plot_center + half_plot;
    Vector3i total_plot_min = plot_min - plot_buffer_vec;
    Vector3i total_plot_max = plot_max + plot_buffer_vec;
    const Vector3 b_d = plot_buffer_vec / Vector3(half_plot + plot_buffer_vec);

    const float max_t = 1.0f;     
    const float core_variation = 1-max_t;

    // Pass 1: Generate terrain heights
    std::vector<int> heightmap(volume_size.x * volume_size.z, base_height);
    for (int z = 0; z < volume_size.z; ++z)
    {
        for (int x = 0; x < volume_size.x; ++x)
        {
            float n = noise->get_noise_2d((float)x, (float)z);
            int noisy_h = base_height + int(n * height_variation);
            int h = noisy_h;

            if (x >= total_plot_min.x && x <= total_plot_max.x &&
                z >= total_plot_min.z && z <= total_plot_max.z)
            {
                // Normalized distance from center, factoring in buffer
                float dx = float(x - plot_center.x) / float(half_plot.x + plot_buffer);
                float dz = float(z - plot_center.z) / float(half_plot.z + plot_buffer);
                float dist = std::max(std::abs(dx), std::abs(dz));
                dist = std::clamp(dist, 0.0f, 1.0f);

                if (dist < 0.5f * (b_d.x + b_d.z))
                {
                    h = base_height + int(n * height_variation * core_variation);
                }
                else
                {
                    float t = (dist - b_d.x) / (1.0f - b_d.x);
                    t = powf(t, 2.0f);
                    t = std::min(t, max_t);
                    h = int(Math::lerp((float)base_height, (float)noisy_h, t));
                }
            }

            heightmap[z * volume_size.x + x] = h;
        }
    }


    // Pass 2: Fill voxels
    for (int z = 0; z < volume_size.z; ++z)
    {
        for (int x = 0; x < volume_size.x; ++x)
        {
            int h = heightmap[z * volume_size.x + x];
            for (int y = 0; y <= h && y < volume_size.y; ++y)
            {
                voxel_data[properties.posToVoxelIndex(Vector3i(x, y, z))] =
                    Voxel::create_voxel(Voxel::VOXEL_TYPE_SOLID, h - y < 1 ? grass_color : ground_color);
            }
        }
    }

    // Pass 3: Call structure generator if provided
    if (structure_pass.is_valid())
    {
        Vector3i bounds_min = {plot_min.x, base_height + 1, plot_min.z};
        Vector3i bounds_max = {plot_max.x, base_height + 1 + flat_plot_size.y, plot_max.z};
        bool success = structure_pass->generate(voxel_data, bounds_min, bounds_max, properties);
        return success;
    }

    return true;
}
