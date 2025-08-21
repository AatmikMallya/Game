#ifndef VOXEL_WORLD_GENERATOR_WFC_NEIGHBORHOOD_H
#define VOXEL_WORLD_GENERATOR_WFC_NEIGHBORHOOD_H

#include <algorithm>
#include <cmath>
#include <godot_cpp/variant/vector3i.hpp>
#include <unordered_map>
#include <vector>

using namespace godot;

struct V3iHash {
    size_t operator()(const Vector3i &v) const noexcept {
        uint64_t x = uint64_t(int64_t(v.x));
        uint64_t y = uint64_t(int64_t(v.y));
        uint64_t z = uint64_t(int64_t(v.z));
        return size_t(x * 73856093ULL ^ y * 19349663ULL ^ z * 83492791ULL);
    }
};

struct Neighborhood {
    virtual ~Neighborhood() = default;
    virtual const std::vector<Vector3i> &offsets() const = 0; // excludes center
    virtual const std::vector<Vector3i> &pattern() const = 0; // includes center
    virtual int get_K() const = 0;
    virtual int opposite_index(int k) const = 0;
    virtual int index_including_center_for(const Vector3i &v) const = 0;
    virtual const std::vector<Vector3i> &center_deltas() const = 0;
    virtual int conditional_index_for_center_delta(const Vector3i &d) const = 0;
};

struct NeighborhoodBase : public Neighborhood {
    int R;
    bool use_exhaustive_offsets_as_deltas;
    std::vector<Vector3i> offs, pat, deltas;
    std::vector<int> opp;
    std::unordered_map<Vector3i, int, V3iHash> pos_to_idx, delta_to_idx;

    NeighborhoodBase(int r, bool use_exhaustive_offsets) : R(r), use_exhaustive_offsets_as_deltas(use_exhaustive_offsets) {}

    template <typename Predicate>
    void build_offsets(Predicate in_neighborhood) {
        for (int z = -R; z <= R; ++z)
            for (int y = -R; y <= R; ++y)
                for (int x = -R; x <= R; ++x) {
                    Vector3i v(x, y, z);
                    if (v == Vector3i(0, 0, 0)) continue;
                    if (in_neighborhood(v)) offs.push_back(v);
                }

        pos_to_idx[Vector3i(0, 0, 0)] = 0;
        for (int i = 0; i < (int)offs.size(); ++i)
            pos_to_idx[offs[i]] = i + 1;

        opp.resize(offs.size());
        for (int k = 0; k < (int)offs.size(); ++k) {
            Vector3i neg(-offs[k].x, -offs[k].y, -offs[k].z);
            auto it = pos_to_idx.find(neg);
            opp[k] = (it == pos_to_idx.end()) ? k : (it->second - 1);
        }

        pat.reserve(1 + offs.size());
        pat.push_back(Vector3i(0, 0, 0));
        pat.insert(pat.end(), offs.begin(), offs.end());

        if (use_exhaustive_offsets_as_deltas) {
            std::vector<Vector3i> S = pat;
            std::unordered_map<Vector3i, bool, V3iHash> seen;
            for (auto &u : S)
                for (auto &w : S) {
                    Vector3i d = w - u;
                    if (d == Vector3i(0, 0, 0)) continue;
                    if (!seen.emplace(d, true).second) continue;
                    deltas.push_back(d);
                }
            std::sort(deltas.begin(), deltas.end(), [](auto &a, auto &b) {
                auto manhattan = [](const Vector3i &v) {
                    return std::abs(v.x) + std::abs(v.y) + std::abs(v.z);
                };
                return manhattan(a) < manhattan(b);
            });
            for (int i = 0; i < (int)deltas.size(); ++i)
                delta_to_idx[deltas[i]] = i;
        } else {
            deltas = offs;
        }
    }

    const std::vector<Vector3i> &offsets() const override { return offs; }
    const std::vector<Vector3i> &pattern() const override { return pat; }
    int get_K() const override { return (int)offs.size(); }
    int opposite_index(int k) const override { return opp[k]; }
    int index_including_center_for(const Vector3i &v) const override {
        auto it = pos_to_idx.find(v);
        return (it == pos_to_idx.end()) ? -1 : it->second;
    }
    const std::vector<Vector3i> &center_deltas() const override { return deltas; }
    int conditional_index_for_center_delta(const Vector3i &d) const override {
        auto it = delta_to_idx.find(d);
        return (it == delta_to_idx.end()) ? -1 : it->second;
    }
};

struct VonNeumann : public NeighborhoodBase {
    explicit VonNeumann(int r, bool use_exhaustive_offsets = false)
        : NeighborhoodBase(r, use_exhaustive_offsets) {
        build_offsets([&](const Vector3i &v) {
            int l1 = std::abs(v.x) + std::abs(v.y) + std::abs(v.z);
            return l1 >= 1 && l1 <= R;
        });
    }
};

struct Moore : public NeighborhoodBase {
    explicit Moore(int r, bool use_exhaustive_offsets = false)
        : NeighborhoodBase(r, use_exhaustive_offsets) {
        build_offsets([&](const Vector3i &v) {
            int l_inf = std::max({std::abs(v.x), std::abs(v.y), std::abs(v.z)});
            return l_inf >= 1 && l_inf <= R;
        });
    }
};

#endif // VOXEL_WORLD_GENERATOR_WFC_NEIGHBORHOOD_H
