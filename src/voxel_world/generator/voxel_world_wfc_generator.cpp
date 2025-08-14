#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <godot_cpp/classes/time.hpp>
#include <limits>
#include <memory>
#include <queue>
#include <random>
#include <vector>

#include "voxel_world_wfc_generator.h"

#include "wfc_neighborhood.h"
#include <godot_cpp/variant/utility_functions.hpp>
using namespace godot;

// Constants
static constexpr int T = 256; // palette size
static constexpr float EPS = 1e-9f;
static constexpr uint8_t DEBUG_TILE_ID = 255; // magenta/debug fallback (adjust to your palette)

namespace
{

// Base class
struct WFCVoxel
{
    enum class Kind : uint8_t
    {
        EMPTY,
        SUPERPOSITION,
        COLLAPSED
    };
    virtual ~WFCVoxel() = default;
    virtual Kind kind() const = 0;
};

// Empty
struct EmptyVoxel : public WFCVoxel
{
    Kind kind() const override
    {
        return Kind::EMPTY;
    }
};

// Superposition (probability vector)
struct SuperpositionVoxel : public WFCVoxel
{
    std::array<float, T> p{};
    float entropy = 0.0f;
    uint32_t version = 0; // bump whenever p changes
    Kind kind() const override
    {
        return Kind::SUPERPOSITION;
    }
};

// Collapsed
struct CollapsedVoxel : public WFCVoxel
{
    uint8_t type = 0;
    bool is_debug = false;
    Kind kind() const override
    {
        return Kind::COLLAPSED;
    }
};

// Helpers
inline int index_3d(const Vector3i &p, const Vector3i &size)
{
    return p.z * (size.x * size.y) + p.y * size.x + p.x;
}

inline bool in_bounds(const Vector3i &p, const Vector3i &size)
{
    return (p.x >= 0 && p.x < size.x && p.y >= 0 && p.y < size.y && p.z >= 0 && p.z < size.z);
}

inline float safe_log(float x)
{
    return std::log(std::max(x, EPS));
}
/*
inline float biased_entropy(const std::array<float, T> &p)
{
    constexpr float w_air = 0.1f; // Air is "less interesting"
    float weighted_sum = 0.0f;
    std::array<float, T> p_adj;

    for (int i = 0; i < T; ++i)
    {
        float w = (i == 0) ? w_air : 1.0f;
        p_adj[i] = p[i] * w;
        weighted_sum += p_adj[i];
    }

    // normalise adjusted probs
    for (int i = 0; i < T; ++i)
        p_adj[i] /= weighted_sum;

    // Shannon entropy on adjusted probs
    float H = 0.0f;
    for (int i = 0; i < T; ++i)
    {
        float pi = p_adj[i];
        if (pi > 0.0f)
            H -= pi * safe_log(pi);
    }
    return H;
}*/

inline float shannon_entropy(const std::array<float, T> &p)
{
    // return biased_entropy(p);
    float H = 0.0f;
    for (int i = 0; i < T; ++i)
    {
        float pi = p[i];
        if (pi > 0.0f)
            H -= pi * safe_log(pi);
    }
    return H;
}

// Normalize vector; returns sum before normalization
inline float normalize(std::array<float, T> &p)
{
    float s = 0.0f;
    for (int i = 0; i < T; ++i)
        s += p[i];
    if (s <= 0.0f)
        return 0.0f;
    float inv = 1.0f / s;
    for (int i = 0; i < T; ++i)
        p[i] *= inv;
    return s;
}

// Weighted sample from p (assumed normalized)
inline int weighted_sample(const std::array<float, T> &p, std::mt19937 &rng)
{
    std::uniform_real_distribution<float> dist(0.0f, 1.0f);
    float r = dist(rng);
    float cum = 0.0f;
    for (int i = 0; i < T; ++i)
    {
        cum += p[i];
        if (r <= cum)
            return i;
    }
    // Fallback in case of FP rounding
    for (int i = T - 1; i >= 0; --i)
        if (p[i] > 0.0f)
            return i;
    return 0;
}

} // namespace

struct WFCModel
{
    // P(center=j | neighbor type t, offset k) or the transpose you prefer
    std::vector<std::array<std::array<float, T>, T>> probabilities{};
    std::array<float, T> priors{};
    const Neighborhood &ngh;

    WFCModel(const Neighborhood &ngh) : ngh(ngh)
    {
        probabilities.resize(ngh.get_K());
    }

    void debug_print(float epsilon = 1e-8f) const
    {
        String s = "Priors (non-zero):\n";
        for (int i = 0; i < T; ++i)
        {
            float p = priors[i];
            if (p > epsilon)
            {
                s += String::num_int64(i) + ": " + String::num(p, 6) + "\n";
            }
        }
        UtilityFunctions::print(s);

        for (int d = 0; d < ngh.get_K(); ++d)
        {
            String block;
            auto offset = ngh.offsets()[d];
            String dir = "(" + String::num_int64(offset.x) + "," + String::num_int64(offset.y) + "," + String::num_int64(offset.z) + ")";
            block += "Dir " + dir + " adjacency (non-zero):\n";
            for (int i = 0; i < T; ++i)
            {
                String line;
                bool any = false;
                for (int j = 0; j < T; ++j)
                {
                    float p = probabilities[d][i][j];
                    if (p > epsilon)
                    {
                        if (!any)
                        {
                            line = String::num_int64(i) + " -> ";
                            any = true;
                        }
                        else
                        {
                            line += String(", ");
                        }
                        line += String::num_int64(j) + ": " + String::num(p, 6);
                    }
                }
                if (any)
                {
                    block += line + "\n";
                }
            }
            UtilityFunctions::print(block);
        }
    }
};

WFCModel build_model_from_voxels(const Ref<VoxelDataVox> voxel_data, const Vector3i &size, const Neighborhood &ngh,
                                 bool use_alpha, float alpha)
{
    WFCModel model = WFCModel(ngh);
    std::vector<std::array<std::array<uint32_t, T>, T>> counts{};
    counts.resize(ngh.get_K());
    std::array<uint32_t, T> prior_counts{};

    auto idx3 = [&](int x, int y, int z) { return z * (size.x * size.y) + y * size.x + x; };

    for (int z = 0; z < size.z; ++z)
        for (int y = 0; y < size.y; ++y)
            for (int x = 0; x < size.x; ++x)
            {
                uint8_t center = voxel_data->get_voxel_index_at(Vector3i(x, y, z));
                prior_counts[center]++;

                const auto &offs = ngh.offsets();
                for (int k = 0; k < ngh.get_K(); ++k)
                {
                    Vector3i o = offs[k];
                    int nx = x + o.x, ny = y + o.y, nz = z + o.z;
                    if (nx < 0 || ny < 0 || nz < 0 || nx >= size.x || ny >= size.y || nz >= size.z)
                        continue;
                    uint8_t neigh = voxel_data->get_voxel_index_at(Vector3i(nx, ny, nz));
                    // counts[k][neighbor_type][center_type] or vice versa — be consistent with apply step
                    counts[k][neigh][center]++; // example: row=neigh type, col=center type
                }
            }

    // Priors with optional alpha
    double tot = 0.0;
    for (int t = 0; t < T; ++t)
    {
        double v = prior_counts[t] + (use_alpha ? alpha : 0.0);
        model.priors[t] = float(v);
        tot += v;
    }
    if (tot > 0)
        for (int t = 0; t < T; ++t)
            model.priors[t] /= float(tot);
    else
        for (int t = 0; t < T; ++t)
            model.priors[t] = 1.0f / T;

    // Normalize conditionals per k, per neighbor type row
    for (int k = 0; k < ngh.get_K(); ++k)
    {
        for (int neigh = 0; neigh < T; ++neigh)
        {
            double row_sum = 0.0;
            for (int center = 0; center < T; ++center)
            {
                double v = counts[k][neigh][center] + (use_alpha ? alpha : 0.0);
                model.probabilities[k][neigh][center] = float(v);
                row_sum += v;
            }
            if (row_sum > 0)
            {
                float inv = float(1.0 / row_sum);
                for (int center = 0; center < T; ++center)
                    model.probabilities[k][neigh][center] *= inv;
            }
            else
            {
                // fallback to priors if no observations
                for (int center = 0; center < T; ++center)
                    model.probabilities[k][neigh][center] = 0; // model.priors[center];
            }
        }
    }

    return model;
}
/*
SuperpositionVoxel *init_from_single_neighbor(int target_idx, std::vector<std::unique_ptr<WFCVoxel>> &grid,
                                              const WFCModel &model, int cond_index, uint8_t neighbor_type,
                                              float weight = 1.0f)
{
    auto sp = std::make_unique<SuperpositionVoxel>();
    for (int j = 0; j < T; ++j)
        sp->p[j] = std::pow(model.probabilities[cond_index][neighbor_type][j], weight);// * model.priors[j];
    if (normalize(sp->p) <= 0.0f)
        return nullptr;
    sp->entropy = shannon_entropy(sp->p);
    sp->version = 0;
    auto *raw = sp.get();
    grid[target_idx] = std::move(sp);
    return raw;
}

bool update_from_neighbor(SuperpositionVoxel &tgt, const WFCModel &model,
                          int cond_index, // k from neighborhood
                          uint8_t neighbor_type, float weight = 1.0f)
{
    bool any = false;
    for (int j = 0; j < T; ++j)
    {
        float before = tgt.p[j];
        float mult = model.probabilities[cond_index][neighbor_type][j];
        // Optional weighting for farther neighbors
        float after = before * std::pow(mult, weight);
        if (after != before)
            any = true;
        tgt.p[j] = after;
    }
    float sum = normalize(tgt.p);
    if (sum <= 0.0f)
        return true; // contradiction; caller decides
    return any;
}*/

void VoxelWorldWFCGenerator::generate(RenderingDevice *rd, VoxelWorldRIDs &voxel_world_rids,
                                      const VoxelWorldProperties &properties)
{
    if (voxel_data.is_null())
    {
        UtilityFunctions::printerr("Voxel data is not set for WFC generator.");
        return;
    }
    voxel_data->load();

    // Decide output size
    Vector3i out_size = Vector3i(properties.grid_size.x, properties.grid_size.y, properties.grid_size.z);
    grid_size = grid_size.min(out_size);

    Vector3i training_size = voxel_data->get_size();

    const bool use_alpha = false;
    const float alpha = 0.25;

    // Moore26Neighborhood ngh = Moore26Neighborhood();
    Face6Neighborhood ngh = Face6Neighborhood();
    const int K = ngh.get_K();

    auto model = build_model_from_voxels(voxel_data, training_size, ngh, use_alpha, alpha);
    model.debug_print();

    // Initialize RNG
    std::mt19937 rng(Time::get_singleton()->get_unix_time_from_system());

    // Grid of polymorphic cells
    const int N = grid_size.x * grid_size.y * grid_size.z;
    std::vector<std::unique_ptr<WFCVoxel>> grid;
    grid.reserve(N);
    for (int i = 0; i < N; ++i)
        grid.emplace_back(std::make_unique<EmptyVoxel>());

    auto get = [&](int idx) -> WFCVoxel * { return grid[idx].get(); };

    // 3) Min-heap with explicit comparator
    struct HeapNode
    {
        float entropy;
        int index;
        uint32_t version;
    };
    struct HeapCompare
    {
        bool operator()(const HeapNode &a, const HeapNode &b) const
        {
            return a.entropy > b.entropy; // min-heap
        }
    };
    std::priority_queue<HeapNode, std::vector<HeapNode>, HeapCompare> heap;

    // 4) Helpers
    auto make_superposition = [&](int idx) -> SuperpositionVoxel * {
        auto sp = std::make_unique<SuperpositionVoxel>();
        for (int j = 1; j < T; ++j)
            sp->p[j] = model.priors[j];
        sp->p[0] = 0.0f;
        normalize(sp->p);
        sp->entropy = shannon_entropy(sp->p);
        sp->version = 0;
        auto *raw = sp.get();
        grid[idx] = std::move(sp);
        return raw;
    };

    auto collapse_to_type = [&](int idx, uint8_t t, bool is_debug) {
        auto cv = std::make_unique<CollapsedVoxel>();
        cv->type = t;
        cv->is_debug = is_debug;
        grid[idx] = std::move(cv);
    };

    auto pos_from_index = [&](int idx) -> Vector3i {
        int x = idx % grid_size.x;
        int y = (idx / grid_size.x) % grid_size.y;
        int z = idx / (grid_size.x * grid_size.y);
        return Vector3i(x, y, z);
    };

    auto init_from_single_neighbor = [&](int target_idx,
                                        const WFCModel &model,
                                        int dir_to_neighbor,
                                        uint8_t neighbor_type,
                                        float weight) -> SuperpositionVoxel* {
        auto sp = std::make_unique<SuperpositionVoxel>();
        for (int j = 0; j < T; ++j)
            sp->p[j] = model.priors[j] * std::pow(model.probabilities[dir_to_neighbor][j][neighbor_type],  weight);

        if (normalize(sp->p) <= 0.0f) {
            collapse_to_type(target_idx, DEBUG_TILE_ID, true); // contradiction marker
            return nullptr;
        }
        sp->entropy = shannon_entropy(sp->p);
        sp->version = 0;
        auto *raw = sp.get();
        grid[target_idx] = std::move(sp);
        return raw;
    };

    // Pass the index in — no pointer arithmetic
    auto update_from_neighbor = [&](int idx,
                                    SuperpositionVoxel &tgt,
                                    const WFCModel &model,
                                    int dir_to_neighbor,
                                    uint8_t neighbor_type,
                                    float weight) -> bool {
        for (int j = 0; j < T; ++j)
            tgt.p[j] *= std::pow(model.probabilities[dir_to_neighbor][j][neighbor_type], weight);

        if (normalize(tgt.p) <= 0.0f) {
            collapse_to_type(idx, DEBUG_TILE_ID, true);
            return false;
        }
        tgt.entropy = shannon_entropy(tgt.p);
        tgt.version += 1;
        return true;
    };

    // 5) Seed Algorithm    
    int seed_idx = index_3d(Vector3i(0,0,0), grid_size);
    SuperpositionVoxel *seed_sp = make_superposition(seed_idx);
    heap.push({seed_sp->entropy, seed_idx, seed_sp->version});

    const auto &offs = ngh.offsets();

    // 6) Main loop
    while (!heap.empty())
    {
        HeapNode node = heap.top();
        heap.pop();
        if (grid[node.index]->kind() != WFCVoxel::Kind::SUPERPOSITION)
            continue;
        auto *sp = static_cast<SuperpositionVoxel *>(grid[node.index].get());
        if (sp->version != node.version)
            continue; // stale

        // Collapse
        int t = weighted_sample(sp->p, rng);
        collapse_to_type(node.index, static_cast<uint8_t>(t), false);

        // Neighbors
        Vector3i pos = pos_from_index(node.index);
        for (int k = 0; k < K; ++k)
        {
            Vector3i np = pos + offs[k];
            if (!in_bounds(np, grid_size))
                continue;
            int nidx = index_3d(np, grid_size);
            float w = ngh.weight_for_offset(k);

            if (grid[nidx]->kind() == WFCVoxel::Kind::EMPTY)
            {
                if (auto *tgt = init_from_single_neighbor(nidx, model, k, t, w))
                    heap.push({tgt->entropy, nidx, tgt->version});
            }
            else if (grid[nidx]->kind() == WFCVoxel::Kind::SUPERPOSITION)
            {
                auto *tgt = static_cast<SuperpositionVoxel *>(grid[nidx].get());
                if (update_from_neighbor(nidx, *tgt, model, k, t, w))
                {
                    // tgt->entropy = shannon_entropy(tgt->p);
                    // tgt->version += 1;
                    heap.push({tgt->entropy, nidx, tgt->version});
                }
            }
        }
    }

    // Convert to output voxels
    std::vector<Voxel> result_voxels = std::vector<Voxel>(voxel_world_rids.voxel_count, Voxel::create_air_voxel());

    for (int i = 0; i < N; ++i)
    {
        // TODO maybe add offsets/scale depending on what we want.
        int result_idx = properties.posToVoxelIndex(pos_from_index(i));
        if (result_idx < 0 || result_idx >= result_voxels.size())
            continue;

        if (grid[i]->kind() == WFCVoxel::Kind::COLLAPSED)
        {
            auto *cv = static_cast<CollapsedVoxel *>(grid[i].get());
            uint8_t id = cv->type;
            // If debug, use a special magenta tile
            if (cv->is_debug)
            {
                result_voxels[result_idx] = Voxel::create_voxel(DEBUG_TILE_ID, Color(1.0f, 0.0f, 1.0f));
            }
            else if (id > 0)
            {
                result_voxels[result_idx] =
                    Voxel::create_voxel(Voxel::VOXEL_TYPE_SOLID, voxel_data->get_palette()[id - 1]);
            }
        }
        // else
        // {
        //     // Uncollapsed/empty: leave as air (or sample priors to fill)
        //     // Optional: finalize by sampling from remaining superposition
        //     if (grid[i]->kind() == WFCVoxel::Kind::SUPERPOSITION)
        //     {
        //         auto *spv = static_cast<SuperpositionVoxel *>(grid[i].get());
        //         int id = weighted_sample(spv->p, rng);
        //         if (id > 0)
        //         {
        //             result_voxels[result_idx] =
        //                 Voxel::create_voxel(Voxel::VOXEL_TYPE_SOLID, voxel_data->get_palette()[id - 1]);
        //         }
        //     }
        // }

        // result_voxels[result_idx] = Voxel::create_voxel(Voxel::VOXEL_TYPE_SOLID, voxel_data->get_palette()[0]);
    }

    voxel_world_rids.set_voxel_data(result_voxels);
}
