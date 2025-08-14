#ifdef DONT_DEFINE
#include <array>
#include <vector>
#include <queue>
#include <random>
#include <memory>
#include <limits>
#include <cmath>
#include <cstdint>
#include <algorithm>
#include <godot_cpp/classes/time.hpp>

#include "voxel_world_wfc_generator.h"

// Constants
static constexpr int T = 256;                 // palette size
static constexpr int DIR_COUNT = 6;
static constexpr float EPS = 1e-9f;
static constexpr uint8_t DEBUG_TILE_ID = 255; // magenta/debug fallback (adjust to your palette)

// Directions: +X, -X, +Y, -Y, +Z, -Z
static const std::array<Vector3i, DIR_COUNT> DIR_OFFSETS = {
    Vector3i(1,0,0), Vector3i(-1,0,0), Vector3i(0,1,0),
    Vector3i(0,-1,0), Vector3i(0,0,1), Vector3i(0,0,-1)
};
static const std::array<int, DIR_COUNT> OPPOSITE_DIR = {1,0,3,2,5,4};

namespace {

// Base class
struct WFCVoxel {
    enum class Kind : uint8_t { EMPTY, SUPERPOSITION, COLLAPSED };
    virtual ~WFCVoxel() = default;
    virtual Kind kind() const = 0;
};

// Empty
struct EmptyVoxel : public WFCVoxel {
    Kind kind() const override { return Kind::EMPTY; }
};

// Superposition (probability vector)
struct SuperpositionVoxel : public WFCVoxel {
    std::array<float, T> p{};
    float entropy = 0.0f;
    uint32_t version = 0; // bump whenever p changes
    Kind kind() const override { return Kind::SUPERPOSITION; }
};

// Collapsed
struct CollapsedVoxel : public WFCVoxel {
    uint8_t type = 0;
    bool is_debug = false;
    Kind kind() const override { return Kind::COLLAPSED; }
};

// Min-heap node
struct HeapNode {
    float entropy;
    int index;
    uint32_t version;
    bool operator>(const HeapNode &other) const {
        if (entropy != other.entropy) return entropy > other.entropy;
        return index > other.index;
    }
};

// Helpers
inline int index_3d(const Vector3i &p, const Vector3i &size) {
    return p.z * (size.x * size.y) + p.y * size.x + p.x;
}

inline bool in_bounds(const Vector3i &p, const Vector3i &size) {
    return (p.x >= 0 && p.x < size.x &&
            p.y >= 0 && p.y < size.y &&
            p.z >= 0 && p.z < size.z);
}

inline float safe_log(float x) {
    return std::log(std::max(x, EPS));
}

inline float shannon_entropy(const std::array<float, T> &p) {
    float H = 0.0f;
    for (int i = 0; i < T; ++i) {
        float pi = p[i];
        if (pi > 0.0f) H -= pi * safe_log(pi);
    }
    return H;
}

// Normalize vector; returns sum before normalization
inline float normalize(std::array<float, T> &p) {
    float s = 0.0f;
    for (int i = 0; i < T; ++i) s += p[i];
    if (s <= 0.0f) return 0.0f;
    float inv = 1.0f / s;
    for (int i = 0; i < T; ++i) p[i] *= inv;
    return s;
}

// Weighted sample from p (assumed normalized)
inline int weighted_sample(const std::array<float, T> &p, std::mt19937 &rng) {
    std::uniform_real_distribution<float> dist(0.0f, 1.0f);
    float r = dist(rng);
    float cum = 0.0f;
    for (int i = 0; i < T; ++i) {
        cum += p[i];
        if (r <= cum) return i;
    }
    // Fallback in case of FP rounding
    for (int i = T - 1; i >= 0; --i) if (p[i] > 0.0f) return i;
    return 0;
}

} // namespace

#include <godot_cpp/variant/utility_functions.hpp>
using namespace godot;

struct WFCModel {
    std::array<std::array<std::array<float, T>, T>, DIR_COUNT> probabilities{};
    std::array<float, T> priors{};

    void debug_print(float epsilon = 1e-8f) const {
        String s;

        s += "Priors (non-zero):\n";
        for (int i = 0; i < T; ++i) {
            float p = priors[i];
            if (p > epsilon) {
                s += String::num_int64(i) + ": " + String::num(p, 6) + "\n";
            }
        }
        UtilityFunctions::print(s);

        for (int d = 0; d < DIR_COUNT; ++d) {
            String block;
            block += "Dir " + String::num_int64(d) + " adjacency (non-zero):\n";
            for (int i = 0; i < T; ++i) {
                String line;
                bool any = false;
                for (int j = 0; j < T; ++j) {
                    float p = probabilities[d][i][j];
                    if (p > epsilon) {
                        if (!any) {
                            line = String::num_int64(i) + " -> ";
                            any = true;
                        } else {
                            line += String(", ");
                        }
                        line += String::num_int64(j) + ": " + String::num(p, 6);
                    }
                }
                if (any) {
                    block += line + "\n";
                }
            }
            UtilityFunctions::print(block);
        }
    }
};

// Build adjacency model from a 3D training volume (template voxels)
static WFCModel build_model_from_voxels(const Ref<VoxelDataVox> &voxel_data,
                                        const Vector3i &size,
                                        bool use_alpha, float alpha) {
    // Count matrices per direction: counts[d][i][j]
    std::array<std::array<std::array<uint32_t, T>, T>, DIR_COUNT> counts{};
    std::array<uint32_t, T> prior_counts{};
    // Iterate volume
    for (int z = 0; z < size.z; ++z) {
        for (int y = 0; y < size.y; ++y) {
            for (int x = 0; x < size.x; ++x) {
                uint8_t center = voxel_data->get_voxel_index_at(Vector3i(x,y,z));// types[idx];
                if(center > 0) prior_counts[center] += 1;
                for (int d = 0; d < DIR_COUNT; ++d) {
                    Vector3i npos = Vector3i(x, y, z) + DIR_OFFSETS[d];
                    if (!in_bounds(npos, size)) continue;                           
                    uint8_t neigh = voxel_data->get_voxel_index_at(npos);
                    counts[d][center][neigh] += 1;
                }
            }
        }
    }

    WFCModel model;

    // Smooth and normalize priors
    double total_prior = 0.0;
    for (int i = 0; i < T; ++i) {
        double v = static_cast<double>(prior_counts[i]);
        if (use_alpha) v += alpha;
        model.priors[i] = static_cast<float>(v);
        total_prior += v;
    }
    if (total_prior > 0.0) {
        for (int i = 0; i < T; ++i) {
            model.priors[i] = static_cast<float>(model.priors[i] / total_prior);
        }
    } else {
        // Uniform fallback
        for (int i = 0; i < T; ++i) model.priors[i] = 1.0f / T;
    }

    // Smooth and normalize adjacency per (d, i)
    for (int d = 0; d < DIR_COUNT; ++d) {
        for (int i = 0; i < T; ++i) {
            double row_sum = 0.0;
            for (int j = 0; j < T; ++j) {
                double v = static_cast<double>(counts[d][i][j]);
                if (use_alpha) v += alpha;
                model.probabilities[d][i][j] = static_cast<float>(v);
                row_sum += v;
            }
            if (row_sum > 0.0) {
                float inv = static_cast<float>(1.0 / row_sum);
                for (int j = 0; j < T; ++j) model.probabilities[d][i][j] *= inv;
            } else {
                // No observations for this row: fall back to priors
                for (int j = 0; j < T; ++j) model.probabilities[d][i][j] = 0;// model.priors[j];
            }
        }
    }

    return model;
}

// Apply constraint from a collapsed neighbor of type t in direction d
// Returns true if distribution changed
static bool apply_neighbor_constraint(SuperpositionVoxel &cell,
                                      const WFCModel &model,
                                      int dir, uint8_t t) {
    bool changed = false;
    float sum_before = 0.0f;
    for (int j = 0; j < T; ++j) sum_before += cell.p[j];

    // Multiply elementwise by row probabilities
    for (int j = 0; j < T; ++j) {
        float old = cell.p[j];
        float mult = model.probabilities[dir][t][j];
        float neu = old * mult;
        if (!changed && std::abs(neu - old) > 1e-12f) changed = true;
        cell.p[j] = neu;
    }

    // Renormalize, clamp tiny values
    float sum = normalize(cell.p);
    if (sum <= 0.0f) {
        return changed; // caller handles contradiction
    }
    // Optional floor to avoid true zeros during propagation
    for (int j = 0; j < T; ++j) if (cell.p[j] < EPS) cell.p[j] = 0.0f;
    normalize(cell.p);

    return changed || (sum_before <= 0.0f);
}

void VoxelWorldWFCGenerator::generate(RenderingDevice *rd,
                                      VoxelWorldRIDs &voxel_world_rids,
                                      const VoxelWorldProperties &properties)
{
    // Safety
    if (voxel_data.is_null()) {
        return;
    }
    voxel_data->load();

    // Decide output size
    Vector3i out_size = Vector3i(properties.grid_size.x, properties.grid_size.y, properties.grid_size.z);
    grid_size = grid_size.min(out_size);

    Vector3i training_size = voxel_data->get_size();
    std::vector<uint8_t> template_voxels = voxel_data->get_voxel_indices();

    std::vector<uint8_t> template_types(training_size.x * training_size.y * training_size.z, 0);
    {
        const size_t n = std::min(template_types.size(), template_voxels.size());
        for (size_t i = 0; i < n; ++i) {
            template_types[i] = template_voxels[i]; // TODO: adapt
        }
    }

    // Build model with optional smoothing
    const bool use_alpha = false;// wfc_use_alpha;       // add to your VoxelWorldProperties
    const float alpha = 0.25; // wfc_alpha;              // e.g., 0.25f
    WFCModel model = build_model_from_voxels(voxel_data, training_size, use_alpha, alpha);
    model.debug_print();

    // Initialize RNG
    std::mt19937 rng(Time::get_singleton()->get_unix_time_from_system());
    
    // Grid of polymorphic cells
    const int N = grid_size.x * grid_size.y * grid_size.z;
    std::vector<std::unique_ptr<WFCVoxel>> grid;
    grid.reserve(N);
    for (int i = 0; i < N; ++i) grid.emplace_back(std::make_unique<EmptyVoxel>());

    auto get = [&](int idx) -> WFCVoxel* { return grid[idx].get(); };

    auto make_superposition = [&](int idx) -> SuperpositionVoxel* {
        auto sp = std::make_unique<SuperpositionVoxel>();
        // Initialize with priors
        for (int j = 0; j < T; ++j) sp->p[j] = model.priors[j];
        normalize(sp->p);
        sp->entropy = shannon_entropy(sp->p);
        SuperpositionVoxel* raw = sp.get();
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

    // Min-heap for selecting next collapse
    std::priority_queue<HeapNode, std::vector<HeapNode>, std::greater<HeapNode>> heap;

    auto enqueue_if_superposition = [&](int idx) {
        if (grid[idx]->kind() == WFCVoxel::Kind::SUPERPOSITION) {
            auto* sp = static_cast<SuperpositionVoxel*>(grid[idx].get());
            heap.push(HeapNode{sp->entropy, idx, sp->version});
        }
    };
    

    // Seed: pick one random position, initialize superposition, and immediately collapse it
    {
        std::uniform_int_distribution<int> px(0, grid_size.x - 1);
        std::uniform_int_distribution<int> py(0, grid_size.y - 1);
        std::uniform_int_distribution<int> pz(0, grid_size.z - 1);
        Vector3i p(px(rng), py(rng), pz(rng));
        int seed_idx = index_3d(p, grid_size);
        SuperpositionVoxel* sp = make_superposition(seed_idx);
        int t = weighted_sample(sp->p, rng);
        collapse_to_type(seed_idx, static_cast<uint8_t>(t), false);
    }

    // Propagation queue (neighbors affected by a collapse)
    std::queue<int> propagate_q;

    /*

    // When a cell collapses, its neighbors are affected
    auto push_neighbors = [&](int idx) {
        Vector3i p = pos_from_index(idx);
        for (int d = 0; d < DIR_COUNT; ++d) {
            Vector3i np = p + DIR_OFFSETS[d];
            if (!in_bounds(np, grid_size)) continue;
            int nidx = index_3d(np, grid_size);
            propagate_q.push(nidx);
        }
    };

    // Seed neighbors propagation
    for (int i = 0; i < N; ++i) {
        if (grid[i]->kind() == WFCVoxel::Kind::COLLAPSED) push_neighbors(i);
    }*/

    // Helper: initialize a superposition from a single neighbor constraint row
    auto init_from_neighbor = [&](int target_idx, const WFCModel &model, int dir, uint8_t neighbor_type) -> SuperpositionVoxel* {
        auto sp = std::make_unique<SuperpositionVoxel>();
        for (int j = 0; j < T; ++j) {
            sp->p[j] = model.probabilities[dir][j][neighbor_type];
        }
        // Optional: if you want to blend in priors slightly, multiply here:
        // for (int j = 0; j < T; ++j) sp->p[j] *= model.priors[j];
        if (normalize(sp->p) <= 0.0f) {
            // If the row was all zeros (hard constraints), fall back to priors or debug
            for (int j = 0; j < T; ++j) sp->p[j] = model.priors[j];
            normalize(sp->p);
        }
        sp->entropy = shannon_entropy(sp->p);
        auto *raw = sp.get();
        grid[target_idx] = std::move(sp);
        return raw;
    };

    auto propagate_once = [&](int target_idx, int from_idx, int dir_from_from_to_target) {
        auto *src = grid[from_idx].get();
        if (!src || src->kind() != WFCVoxel::Kind::COLLAPSED) return;

        uint8_t neigh_type = static_cast<CollapsedVoxel*>(src)->type;

        if (!grid[target_idx]) return;

        if (grid[target_idx]->kind() == WFCVoxel::Kind::EMPTY) {
            auto *tgt = init_from_neighbor(target_idx, model, dir_from_from_to_target, neigh_type);
            tgt->version += 1;
            heap.push(HeapNode{tgt->entropy, target_idx, tgt->version});
            return;
        }

        if (grid[target_idx]->kind() != WFCVoxel::Kind::SUPERPOSITION) return;

        auto *tgt = static_cast<SuperpositionVoxel*>(grid[target_idx].get());
        bool changed = apply_neighbor_constraint(*tgt, model, dir_from_from_to_target, neigh_type);
        if (!changed) return;

        float sum = 0.0f;
        for (int j = 0; j < T; ++j) sum += tgt->p[j];
        if (sum <= 0.0f) {
            collapse_to_type(target_idx, DEBUG_TILE_ID, true);
            return;
        }
        tgt->entropy = shannon_entropy(tgt->p);
        tgt->version += 1;
        heap.push(HeapNode{tgt->entropy, target_idx, tgt->version});
    };


    /*
    // Propagate constraints from all current collapsed cells
    while (!propagate_q.empty()) {
        int nidx = propagate_q.front(); propagate_q.pop();
        if (grid[nidx]->kind() == WFCVoxel::Kind::COLLAPSED) continue;

        Vector3i np = pos_from_index(nidx);
        for (int d = 0; d < DIR_COUNT; ++d) {
            Vector3i sp_pos = np + DIR_OFFSETS[d];
            if (!in_bounds(sp_pos, grid_size)) continue;
            int sidx = index_3d(sp_pos, grid_size);
            if (grid[sidx]->kind() == WFCVoxel::Kind::COLLAPSED) {
                // Neighbor sidx collapsed; it constrains nidx using opposite dir
                propagate_once(nidx, sidx, OPPOSITE_DIR[d]);
            }
        }
        enqueue_if_superposition(nidx);
    }

    // Collapse until grid filled
    int collapsed_count = 0;
    for (int i = 0; i < N; ++i) if (grid[i]->kind() == WFCVoxel::Kind::COLLAPSED) collapsed_count++;*/

    int collapsed_count = 0;

    while (collapsed_count < N) {
        // Get next superposition with min entropy
        int idx = -1;
        while (!heap.empty()) {
            HeapNode node = heap.top(); heap.pop();
            if (grid[node.index]->kind() != WFCVoxel::Kind::SUPERPOSITION) continue;
            auto* sp = static_cast<SuperpositionVoxel*>(grid[node.index].get());
            if (sp->version != node.version) continue; // stale
            idx = node.index;
            break;
        }

        if (idx == -1) {
            break;
            // // No candidate superpositions left; initialize a new random empty cell to keep going
            // int fallback = -1;
            // for (int i = 0; i < N; ++i) {
            //     if (grid[i]->kind() == WFCVoxel::Kind::EMPTY) { fallback = i; break; }
            // }
            // if (fallback == -1) break; // done
            // SuperpositionVoxel* sp = make_superposition(fallback);
            // heap.push(HeapNode{sp->entropy, fallback, sp->version});
            // continue;
        }

        // Collapse it
        auto* sp = static_cast<SuperpositionVoxel*>(grid[idx].get());
        int t = weighted_sample(sp->p, rng);
        collapse_to_type(idx, static_cast<uint8_t>(t), false);
        collapsed_count += 1;

        // Propagate to its neighbors
        Vector3i p = pos_from_index(idx);
        for (int d = 0; d < DIR_COUNT; ++d) {
            Vector3i np = p + DIR_OFFSETS[d];
            if (!in_bounds(np, grid_size)) continue;
            int nidx = index_3d(np, grid_size);
            // push immediate update influenced by this collapse
            propagate_once(nidx, idx, OPPOSITE_DIR[d]);
            // and schedule neighbors for further propagation
            propagate_q.push(nidx);
        }

        // Drain propagation queue from this collapse
        while (!propagate_q.empty()) {
            int nidx2 = propagate_q.front(); propagate_q.pop();
            if (grid[nidx2]->kind() == WFCVoxel::Kind::COLLAPSED) continue;
            Vector3i np2 = pos_from_index(nidx2);
            for (int d = 0; d < DIR_COUNT; ++d) {
                Vector3i sp_pos2 = np2 + DIR_OFFSETS[d];
                if (!in_bounds(sp_pos2, grid_size)) continue;
                int sidx2 = index_3d(sp_pos2, grid_size);
                if (grid[sidx2]->kind() == WFCVoxel::Kind::COLLAPSED) {
                    propagate_once(nidx2, sidx2, OPPOSITE_DIR[d]);
                }
            }
            enqueue_if_superposition(nidx2);
        }
    }

    // Convert to output voxels
    std::vector<Voxel> result_voxels = std::vector<Voxel>(voxel_world_rids.voxel_count, Voxel::create_air_voxel());

    for (int i = 0; i < N; ++i) {
        //TODO maybe add offsets/scale depending on what we want.
        int result_idx = properties.posToVoxelIndex(pos_from_index(i));
        if(result_idx < 0 || result_idx >= result_voxels.size()) continue;

        if (grid[i]->kind() == WFCVoxel::Kind::COLLAPSED) {
            auto* cv = static_cast<CollapsedVoxel*>(grid[i].get());
            uint8_t id = cv->type;
            // If debug, use a special magenta tile
            if (cv->is_debug) {
                result_voxels[result_idx] = Voxel::create_voxel(DEBUG_TILE_ID, Color(1.0f, 0.0f, 1.0f));
            } else if(id > 0) {
                result_voxels[result_idx] = Voxel::create_voxel(Voxel::VOXEL_TYPE_SOLID, voxel_data->get_palette()[id - 1]);
            }
        } else {
            // Uncollapsed/empty: leave as air (or sample priors to fill)
            // Optional: finalize by sampling from remaining superposition
            if (grid[i]->kind() == WFCVoxel::Kind::SUPERPOSITION) {
                auto* spv = static_cast<SuperpositionVoxel*>(grid[i].get());
                int id = weighted_sample(spv->p, rng);
                if (id > 0) {
                    result_voxels[result_idx] = Voxel::create_voxel(Voxel::VOXEL_TYPE_SOLID, voxel_data->get_palette()[id - 1]);
                }
            }
        }

        // result_voxels[result_idx] = Voxel::create_voxel(Voxel::VOXEL_TYPE_SOLID, voxel_data->get_palette()[0]);
    }

    voxel_world_rids.set_voxel_data(result_voxels);
}
#endif