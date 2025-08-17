#ifndef VOXEL_WORLD_GENERATOR_WFC_NEIGHBORHOOD_H
#define VOXEL_WORLD_GENERATOR_WFC_NEIGHBORHOOD_H

#include <algorithm>
#include <cmath>
#include <godot_cpp/variant/vector3i.hpp>
#include <unordered_map>
#include <vector>

using namespace godot;

struct V3iHash
{
    size_t operator()(const Vector3i &v) const noexcept
    {
        uint64_t x = uint64_t(int64_t(v.x));
        uint64_t y = uint64_t(int64_t(v.y));
        uint64_t z = uint64_t(int64_t(v.z));
        return size_t(x * 73856093ULL ^ y * 19349663ULL ^ z * 83492791ULL);
    }
};

struct Neighborhood
{
    virtual ~Neighborhood() = default;

    virtual const std::vector<Vector3i> &offsets() const = 0; // excludes center
    virtual int get_K() const = 0;                            // number of offsets
    virtual int opposite_index(int k) const = 0;              // opposite offset index

    // 0=center, 1..K = offsets()[i], -1 if outside footprint
    virtual int index_including_center_for(const Vector3i &v) const = 0;

    // (S - S) \ {0}, where S = {0} ∪ offsets()
    virtual const std::vector<Vector3i> &center_deltas() const = 0;
    virtual int conditional_index_for_center_delta(const Vector3i &d) const = 0;
};

// ------------------------------------------------------------
// VonNeumann Neighborhood: L1 distance 1..R
// ------------------------------------------------------------
struct VonNeumann : public Neighborhood
{
    int R;
    bool use_offsets_as_deltas;
    std::vector<Vector3i> offs;
    std::vector<int> opp;
    std::unordered_map<Vector3i, int, V3iHash> pos_to_idx;
    std::vector<Vector3i> deltas;
    std::unordered_map<Vector3i, int, V3iHash> delta_to_idx;

    explicit VonNeumann(int r, bool use_offsets = false) : R(r), use_offsets_as_deltas(use_offsets)
    {
        // build offsets: 1 <= L1 <= R
        for (int z = -R; z <= R; ++z)
            for (int y = -R; y <= R; ++y)
                for (int x = -R; x <= R; ++x)
                {
                    int l1 = std::abs(x) + std::abs(y) + std::abs(z);
                    if (l1 >= 1 && l1 <= R)
                        offs.push_back(Vector3i(x, y, z));
                }

        // footprint index
        pos_to_idx[Vector3i(0, 0, 0)] = 0;
        for (int i = 0; i < (int)offs.size(); ++i)
            pos_to_idx[offs[i]] = i + 1;

        // opposite mapping
        opp.resize(offs.size());
        for (int k = 0; k < (int)offs.size(); ++k)
        {
            Vector3i neg(-offs[k].x, -offs[k].y, -offs[k].z);
            auto it = pos_to_idx.find(neg);
            opp[k] = (it == pos_to_idx.end()) ? k : (it->second - 1);
        }

        if (use_offsets_as_deltas)
            deltas = offs;
        else
        {
            // deltas from S−S
            std::vector<Vector3i> S;
            S.reserve(1 + offs.size());
            S.push_back(Vector3i(0, 0, 0));
            for (auto &o : offs)
                S.push_back(o);
            std::unordered_map<Vector3i, bool, V3iHash> seen;
            for (auto &u : S)
                for (auto &w : S)
                {
                    Vector3i d = w - u;
                    if (d == Vector3i(0, 0, 0))
                        continue;
                    if (!seen.emplace(d, true).second)
                        continue;
                    deltas.push_back(d);
                }
            std::sort(deltas.begin(), deltas.end(), [](auto &a, auto &b) {
                if (a.x != b.x)
                    return a.x < b.x;
                if (a.y != b.y)
                    return a.y < b.y;
                return a.z < b.z;
            });
            for (int i = 0; i < (int)deltas.size(); ++i)
                delta_to_idx[deltas[i]] = i;
        }
    }

    const std::vector<Vector3i> &offsets() const override
    {
        return offs;
    }
    int get_K() const override
    {
        return (int)offs.size();
    }
    int opposite_index(int k) const override
    {
        return opp[k];
    }

    int index_including_center_for(const Vector3i &v) const override
    {
        auto it = pos_to_idx.find(v);
        return (it == pos_to_idx.end()) ? -1 : it->second;
    }

    const std::vector<Vector3i> &center_deltas() const override
    {
        return deltas;
    }
    int conditional_index_for_center_delta(const Vector3i &d) const override
    {
        auto it = delta_to_idx.find(d);
        return (it == delta_to_idx.end()) ? -1 : it->second;
    }
};

// ------------------------------------------------------------
// Moore Neighborhood: Chebyshev distance 1..R
// ------------------------------------------------------------
struct Moore : public Neighborhood
{
    int R;
    bool use_offsets_as_deltas;
    std::vector<Vector3i> offs;
    std::vector<int> opp;
    std::unordered_map<Vector3i, int, V3iHash> pos_to_idx;
    std::vector<Vector3i> deltas;
    std::unordered_map<Vector3i, int, V3iHash> delta_to_idx;

    explicit Moore(int r, bool use_offsets=false)
        : R(r), use_offsets_as_deltas(use_offsets)
    {
        // build offsets: 1 <= L∞ <= R
        for (int z = -R; z <= R; ++z)
            for (int y = -R; y <= R; ++y)
                for (int x = -R; x <= R; ++x)
                {
                    if (x == 0 && y == 0 && z == 0)
                        continue;
                    int l_inf = std::max({std::abs(x), std::abs(y), std::abs(z)});
                    if (l_inf >= 1 && l_inf <= R)
                        offs.push_back(Vector3i(x, y, z));
                }

        // footprint index
        pos_to_idx[Vector3i(0, 0, 0)] = 0;
        for (int i = 0; i < (int)offs.size(); ++i)
            pos_to_idx[offs[i]] = i + 1;

        // opposite mapping
        opp.resize(offs.size());
        for (int k = 0; k < (int)offs.size(); ++k)
        {
            Vector3i neg(-offs[k].x, -offs[k].y, -offs[k].z);
            auto it = pos_to_idx.find(neg);
            opp[k] = (it == pos_to_idx.end()) ? k : (it->second - 1);
        }

        if (use_offsets_as_deltas)
            deltas = offs;
        else
        {

            // deltas from S−S
            std::vector<Vector3i> S;
            S.reserve(1 + offs.size());
            S.push_back(Vector3i(0, 0, 0));
            for (auto &o : offs)
                S.push_back(o);
            std::unordered_map<Vector3i, bool, V3iHash> seen;
            for (auto &u : S)
                for (auto &w : S)
                {
                    Vector3i d = w - u;
                    if (d == Vector3i(0, 0, 0))
                        continue;
                    if (!seen.emplace(d, true).second)
                        continue;
                    deltas.push_back(d);
                }
            std::sort(deltas.begin(), deltas.end(), [](auto &a, auto &b) {
                if (a.x != b.x)
                    return a.x < b.x;
                if (a.y != b.y)
                    return a.y < b.y;
                return a.z < b.z;
            });
            for (int i = 0; i < (int)deltas.size(); ++i)
                delta_to_idx[deltas[i]] = i;
        }
    }

    const std::vector<Vector3i> &offsets() const override
    {
        return offs;
    }
    int get_K() const override
    {
        return (int)offs.size();
    }
    int opposite_index(int k) const override
    {
        return opp[k];
    }

    int index_including_center_for(const Vector3i &v) const override
    {
        auto it = pos_to_idx.find(v);
        return (it == pos_to_idx.end()) ? -1 : it->second;
    }

    const std::vector<Vector3i> &center_deltas() const override
    {
        return deltas;
    }
    int conditional_index_for_center_delta(const Vector3i &d) const override
    {
        auto it = delta_to_idx.find(d);
        return (it == delta_to_idx.end()) ? -1 : it->second;
    }
};

#endif // VOXEL_WORLD_GENERATOR_WFC_NEIGHBORHOOD_H
