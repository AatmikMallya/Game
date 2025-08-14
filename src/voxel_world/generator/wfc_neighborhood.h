#ifndef VOXEL_WORLD_GENERATOR_WFC_NEIGHBORHOOD_H
#define VOXEL_WORLD_GENERATOR_WFC_NEIGHBORHOOD_H

#include <array>
#include <vector>

#include <godot_cpp/variant/vector3.hpp>
#include <godot_cpp/variant/vector3i.hpp>

using namespace godot;

struct Neighborhood
{
    // Return list of relative offsets to consider (no {0,0,0})
    virtual const std::vector<Vector3i> &offsets() const = 0;

    // Map offset index to a conditional index in the model.
    // For 6-face, you'd return 0..5; for 26, define your own indexing.
    virtual int conditional_index_for_offset(int k) const = 0;

    // Optional: strength per offset (1.0 default)
    virtual float weight_for_offset(int k) const
    {
        return 1.0f;
    }

    virtual int get_K() const
    {
        return 0;
    }

    virtual ~Neighborhood() = default;
};

struct Face6Neighborhood : public Neighborhood
{
    std::vector<Vector3i> offs;
    Face6Neighborhood()
    {
        offs = {{1, 0, 0}, {-1, 0, 0}, {0, 1, 0}, {0, -1, 0}, {0, 0, 1}, {0, 0, -1}};
    }
    const std::vector<Vector3i> &offsets() const override
    {
        return offs;
    }
    int conditional_index_for_offset(int k) const override
    {
        return k;
    } // 0..5
    int get_K() const override
    {
        return 6;
    }
};

struct Moore26Neighborhood : public Neighborhood
{
    std::vector<Vector3i> offs;
    std::array<int, 26> cond_index; // your custom mapping
    Moore26Neighborhood()
    {
        offs.reserve(26);
        for (int dz = -1; dz <= 1; ++dz)
            for (int dy = -1; dy <= 1; ++dy)
                for (int dx = -1; dx <= 1; ++dx)
                {
                    if (dx == 0 && dy == 0 && dz == 0)
                        continue;
                    offs.push_back(Vector3i(dx, dy, dz));
                }
        // Fill cond_index with a stable mapping (see model below)
        for (int k = 0; k < 26; ++k)
            cond_index[k] = k; // simple: 0..25
    }
    const std::vector<Vector3i> &offsets() const override
    {
        return offs;
    }
    int conditional_index_for_offset(int k) const override
    {
        return cond_index[k];
    }
    float weight_for_offset(int k) const override
    {
        return 1.0f;
        Vector3i o = offs[k];
        int m = std::abs(o.x) + std::abs(o.y) + std::abs(o.z);
        // Example: weaker influence for edges/corners
        return (m == 1) ? 1.0f : (m == 2 ? 0.7f : 0.5f);
    }
    int get_K() const override
    {
        return 26;
    }
};

#endif // VOXEL_WORLD_GENERATOR_WFC_NEIGHBORHOOD_H