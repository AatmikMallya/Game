#ifndef VOXEL_WORLD_TERRAIN_GENERATOR_H
#define VOXEL_WORLD_TERRAIN_GENERATOR_H

#include <godot_cpp/classes/node.hpp>
#include <godot_cpp/classes/fast_noise_lite.hpp>
#include <godot_cpp/core/class_db.hpp>
#include "voxel_world/voxel_properties.h"
#include "voxel_world/generator/voxel_world_generator_cpu_pass.h"

using namespace godot;

class VoxelWorldTerrainGenerator : public VoxelWorldGeneratorCPUPass {
    GDCLASS(VoxelWorldTerrainGenerator, VoxelWorldGeneratorCPUPass)

public:
    VoxelWorldTerrainGenerator() = default;
    ~VoxelWorldTerrainGenerator() override = default;



    // Main API
    bool generate(std::vector<Voxel> &voxel_data, const Vector3i bounds_min, const Vector3i bounds_max, const VoxelWorldProperties &properties) override;

    // --- Getters/Setters ---
    void set_noise_frequency(float p_freq) { noise_frequency = p_freq; }
    float get_noise_frequency() const { return noise_frequency; }

    void set_noise_octaves(int p_oct) { noise_octaves = p_oct; }
    int get_noise_octaves() const { return noise_octaves; }

    void set_base_height(int p_h) { base_height = p_h; }
    int get_base_height() const { return base_height; }

    void set_height_variation(int p_var) { height_variation = p_var; }
    int get_height_variation() const { return height_variation; }

    void set_flat_plot_size(Vector3i p_size) { flat_plot_size = p_size; }
    Vector3i get_flat_plot_size() const { return flat_plot_size; }

    void set_plot_center(Vector3i p_center) { plot_center = p_center; }
    Vector3i get_plot_center() const { return plot_center; }

    void set_ground_color(Color p_color) { ground_color = p_color; }
    Color get_ground_color() const { return ground_color; }

    void set_grass_color(Color p_color) { grass_color = p_color; }
    Color get_grass_color() const { return grass_color; }

    void set_structure_pass(Ref<VoxelWorldGeneratorCPUPass> p_structure_pass) { structure_pass = p_structure_pass; }
    Ref<VoxelWorldGeneratorCPUPass> get_structure_pass() { return structure_pass; }
    

protected:
    static void _bind_methods();

private:
    // Noise parameters
    float noise_frequency = 0.02f;
    int noise_octaves = 4;

    // Height parameters
    int base_height = 8;
    int height_variation = 8;

    // Flat plot
    Vector3i flat_plot_size = {16, 0, 16};
    int plot_buffer = 64;
    Vector3i plot_center = {32, 0, 32};

    // Appearance
    Color ground_color = Color(0.4f, 0.25f, 0.1f);
    Color grass_color = Color(0.2f, 0.65f, 0.2f);


    Ref<VoxelWorldGeneratorCPUPass> structure_pass;
};

#endif // VOXEL_WORLD_TERRAIN_GENERATOR_H
