// voxel_world_wfc_pattern_generator.cpp

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <godot_cpp/classes/time.hpp>
#include <godot_cpp/variant/utility_functions.hpp>
#include <memory>
#include <queue>
#include <random>
#include <string>
#include <unordered_map>
#include <vector>

#include "voxel_world_wfc_pattern_generator.h"
#include "wfc_neighborhood.h" // provides Neighborhood interface and concrete Face6/Moore26 with offsets(), opposite_index(), get_K()

using namespace godot;

// -------------------- Reused utilities (from adjacency WFC) --------------------

static constexpr float EPS = 1e-9f;

inline float safe_log(float x)
{
    return std::log(std::max(x, EPS));
}

// Shannon entropy over a dynamic vector
inline float shannon_entropy(const std::vector<float> &p)
{
    float H = 0.0f;
    for (float pi : p)
    {
        if (pi > 0.0f)
            H -= pi * safe_log(pi);
    }
    return H;
}

// Normalize; returns original sum
inline float normalize(std::vector<float> &p)
{
    float s = 0.0f;
    for (float v : p)
        s += v;
    if (s <= 0.0f)
        return 0.0f;
    const float inv = 1.0f / s;
    for (float &v : p)
        v *= inv;
    return s;
}

inline int weighted_sample(const std::vector<float> &p, std::mt19937 &rng)
{
    std::uniform_real_distribution<float> dist(0.0f, 1.0f);
    float r = dist(rng);
    float cum = 0.0f;
    const int n = static_cast<int>(p.size());
    for (int i = 0; i < n; ++i)
    {
        cum += p[i];
        if (r <= cum)
            return i;
    }
    for (int i = n - 1; i >= 0; --i)
        if (p[i] > 0.0f)
            return i;
    return 0;
}

// -------------------- Pattern data structures --------------------

struct Pattern
{
    std::vector<Voxel> voxels; // size = 1 + K
};

struct PatternKeyHash
{
    size_t operator()(const Pattern &p) const noexcept
    {
        uint64_t h = 1469598103934665603ULL; // FNV-1a 64-bit
        for (const Voxel &v : p.voxels)
        {
            uint32_t d = static_cast<uint32_t>(v.data);
            h ^= d;
            h *= 1099511628211ULL;
        }
        return size_t(h);
    }
};

struct PatternKeyEq
{
    bool operator()(const Pattern &a, const Pattern &b) const noexcept
    {
        return a.voxels == b.voxels;
    }
};

struct PatternModel
{
    int K = 0; //amount of offsets
    int D = 0; //amount of center deltas
    const Neighborhood *ngh = nullptr; // not owned

    std::vector<Pattern> patterns; // size P
    std::vector<float> priors;     // size P

    // Compatibility as bit masks: compat[k][a] is P bits packed in bytes
    // It essentially stores a P x P boolean compatability matrix for each direct offset k.
    // compat[k][a][b] == 1 means pattern b can be placed in direction k relative to a without conflict.
    std::vector<std::vector<std::vector<uint64_t>>> compat; // [K][P][Pbits/64]

    bool is_compatible(int k, int a, int b) const
    {
        if (k < 0 || k >= K || a < 0 || a >= static_cast<int>(patterns.size()) || b < 0 ||
            b >= static_cast<int>(patterns.size()))
            return false;
        const auto &mask = compat[k][a];
        int byte_index = b >> 6;
        int bit_index = b & 63;
        return (mask[byte_index] & (uint64_t(1) << bit_index)) != 0;
    }
};

// -------------------- Grid cells (pattern WFC) --------------------

struct PatternCell
{
    enum class Kind : uint8_t
    {
        EMPTY,
        SUPERPOSITION,
        COLLAPSED
    };
    virtual ~PatternCell() = default;
    virtual Kind kind() const = 0;
};

struct EmptyCell : public PatternCell
{
    Kind kind() const override
    {
        return Kind::EMPTY;
    }
};

struct SuperpositionCell : PatternCell
{
    std::vector<uint64_t> domain_bits; // (P+7)/8 bytes
    std::vector<float> p;              // length P, for sampling
    float entropy = 0.0f;
    uint32_t version = 0; // if ~0, this cell is invalid
    Kind kind() const override
    {
        return Kind::SUPERPOSITION;
    }
};

struct CollapsedCell : public PatternCell
{
    int pattern_id = -1; // 0..P-1
    bool is_debug = false;
    Kind kind() const override
    {
        return Kind::COLLAPSED;
    }
};

// -------------------- Neighborhood helpers --------------------

inline int index_3d(const Vector3i &p, const Vector3i &size)
{
    return p.z * (size.x * size.y) + p.y * size.x + p.x;
}

inline bool in_bounds(const Vector3i &p, const Vector3i &size)
{
    return p.x >= 0 && p.x < size.x && p.y >= 0 && p.y < size.y && p.z >= 0 && p.z < size.z;
}

// -------------------- Pattern extraction and compatibility --------------------

static Pattern extract_pattern_voxels(const Ref<VoxelDataVox> &vox, const Vector3i &center, const Neighborhood &ngh) {
    Pattern pat;
    pat.voxels.reserve(1 + ngh.get_K());
    pat.voxels.push_back(vox->get_voxel_at(center));
    for (auto &off : ngh.offsets()) pat.voxels.push_back(vox->get_voxel_at(center + off));
    return pat;
}

static bool patterns_compatible_delta(const Neighborhood &ngh, const Pattern &A, const Pattern &B,
                                      const Vector3i &delta)
{
    // S = {0} ∪ offsets(); A/B.voxels are indexed: 0=center, 1..K = offsets()[i]
    const auto &offs = ngh.offsets();
    const int K = (int)offs.size();

    // center in A maps to -delta in B
    {
        int idxB = ngh.index_including_center_for(Vector3i(-delta.x, -delta.y, -delta.z));
        if (idxB >= 0 && A.voxels[0] != B.voxels[idxB]) return false;
    }

    for (int i = 0; i < K; ++i) {
        const Vector3i posA = offs[i];
        const Vector3i posB = posA - delta;
        int idxB = ngh.index_including_center_for(posB);
        if (idxB >= 0) {
            if (A.voxels[i + 1] != B.voxels[idxB]) return false;
        }
    }
    return true;
}


static PatternModel build_pattern_model_from_neighborhood(const Ref<VoxelDataVox> &vox, const Vector3i &size,
                                                          const Neighborhood &ngh)
{
    PatternModel model;
    model.K = ngh.get_K();
    model.D = (int)ngh.center_deltas().size();
    model.ngh = &ngh;

    std::unordered_map<Pattern, int, PatternKeyHash, PatternKeyEq> pattern_ids;
    std::vector<uint32_t> counts;

    // 1) Extract unique patterns fully in bounds
    for (int z = 0; z < size.z; ++z)
    for (int y = 0; y < size.y; ++y)
    for (int x = 0; x < size.x; ++x) {
        Vector3i c(x, y, z);
        bool all_in = in_bounds(c, size);
        if (all_in) {
            for (auto &off : ngh.offsets()) {
                if (!in_bounds(c + off, size)) { all_in = false; break; }
            }
        }
        if (!all_in) continue;

        Pattern pat = extract_pattern_voxels(vox, c, ngh);
        auto it = pattern_ids.find(pat);
        if (it == pattern_ids.end()) {
            int id = (int)model.patterns.size();
            pattern_ids.emplace(pat, id);
            model.patterns.push_back(std::move(pat));
            counts.push_back(1);
        } else {
            counts[it->second] += 1;
        }
    }

    const int P = (int)model.patterns.size();
    UtilityFunctions::print(String("Extracted ") + String::num_int64(P) + " unique patterns");
    model.priors.resize(P, 0.0f);
    uint64_t total = 0; for (auto cnt : counts) total += cnt;
    for (int i = 0; i < P; ++i) model.priors[i] = total ? float(counts[i]) / float(total) : 0.0f;

    // 2) Compatibility per delta: compat_delta[D][P][words]
    const int words_per_mask = (P + 63) / 64;
    model.compat.assign(model.D,
        std::vector<std::vector<uint64_t>>(P, std::vector<uint64_t>(words_per_mask, 0)));

    for (int d = 0; d < model.D; ++d) {
        const Vector3i delta = ngh.center_deltas()[d];
        for (int a = 0; a < P; ++a) {
            auto &mask = model.compat[d][a];
            for (int b = 0; b < P; ++b) {
                if (patterns_compatible_delta(*model.ngh, model.patterns[a], model.patterns[b], delta)) {
                    mask[b >> 6] |= (uint64_t(1) << (b & 63));
                }
            }
        }
    }

    return model;
}


void debug_place_and_print_patterns(PatternModel &model, const Neighborhood &ngh, VoxelWorldRIDs &voxel_world_rids,
                                    const VoxelWorldProperties &properties)
{
    float epsilon = 1e-8f;
    int P = static_cast<int>(model.patterns.size());

    const auto &grid_size = properties.grid_size; // assume Vector3i or similar
    std::vector<Voxel> result_voxels(voxel_world_rids.voxel_count, Voxel::create_air_voxel());

    // --- 1) Pattern placement in debug order ---
    int gap_x = 1, gap_y = 2, gap_z = 3;

    Vector3i cursor(0, 0, 0);
    for (int pid = 0; pid < P; ++pid)
    {
        const auto &pat = model.patterns[pid];
        Vector3i pat_size = Vector3i(3, 3, 3); // you need a way to know pattern voxel extents

        // Skip if it doesn't fit at current cursor
        if (cursor.x + pat_size.x > grid_size.x || cursor.y + pat_size.y > grid_size.y ||
            cursor.z + pat_size.z > grid_size.z)
        {
            // Move to next row/column
            cursor.x = 0;
            cursor.y += pat_size.y + gap_y;

            // If Y overflow, reset Y and go up in Z
            if (cursor.y + pat_size.y > grid_size.y)
            {
                cursor.y = 0;
                cursor.z += pat_size.z + gap_z;
            }

            // Check final fit after advancing
            if (cursor.z + pat_size.z > grid_size.z)
                break; // world full in Z
        }

        // Place pattern voxels
        for (int k = 0; k < ngh.get_K() + 1; ++k)
        {
            Vector3i xyz = Vector3i(1, 1, 1) + ((k == 0) ? Vector3i(0, 0, 0) : ngh.offsets()[k - 1]);
            int idx = properties.posToVoxelIndex(cursor + xyz);
            result_voxels[idx] = pat.voxels[k];
        }

        // Advance X
        cursor.x += pat_size.x + gap_x;

        // Wrap X
        if (cursor.x + pat_size.x > grid_size.x)
        {
            cursor.x = 0;
            cursor.y += pat_size.y + gap_y;
            if (cursor.y + pat_size.y > grid_size.y)
            {
                cursor.y = 0;
                cursor.z += pat_size.z + gap_z;
            }
        }
    }

    voxel_world_rids.set_voxel_data(result_voxels);

    // --- 2) Pattern priors ---
    String s = "Priors (non-zero):\n";
    for (int i = 0; i < P; ++i)
    {
        float p = model.priors[i];
        if (p > epsilon)
            s += String::num_int64(i) + ": " + String::num(p, 6) + "\n";
    }
    UtilityFunctions::print(s);

    // --- 3) Adjacency per direction ---
    for (int d = 0; d < ngh.get_K(); ++d)
    {
        String block;
        auto offset = ngh.offsets()[d];
        String dir = "(" + String::num_int64(offset.x) + "," + String::num_int64(offset.y) + "," +
                     String::num_int64(offset.z) + ")";
        block += "Dir " + dir + " adjacency (non-zero):\n";

        for (int i = 0; i < P; ++i)
        {
            String line;
            bool any = false;
            for (int j = 0; j < P; ++j)
            {
                if (model.is_compatible(d, i, j))
                {
                    if (!any)
                    {
                        line = String::num_int64(i) + " -> ";
                        any = true;
                    }
                    else
                        line += String(", ");

                    line += String::num_int64(j);
                }
            }
            if (any)
                block += line + "\n";
        }
        UtilityFunctions::print(block);
    }
}

void debug_place_pattern_pairs(PatternModel &model, const Neighborhood &ngh, VoxelWorldRIDs &voxel_world_rids,
                               const VoxelWorldProperties &properties, int N_examples, uint32_t rng_seed = 12345)
{
    int P = static_cast<int>(model.patterns.size());
    if (P == 0)
        return;

    const auto &grid_size = properties.grid_size;
    std::vector<Voxel> result_voxels(voxel_world_rids.voxel_count, Voxel::create_air_voxel());

    // Base spacing + extra padding around each 2‑pattern block
    const int gap_x = 1 + 1; // +1 padding around pair
    const int gap_y = 2 + 2;
    const int gap_z = 3 + 3;

    Vector3i pat_size(3, 3, 3);     // same for all patterns here
    Vector3i block_size = pat_size; // will enlarge below for pair

    // A pair's bounding box in placement space = both patterns in one dir
    // If you place neighbor along X, width = 2*pat_size.x
    // We'll be conservative and just add pat_size in largest dimension + gap
    block_size.x = pat_size.x * 2;
    block_size.y = pat_size.y * 2;
    block_size.z = pat_size.z * 2;

    Vector3i cursor(0, 0, 0);

    std::mt19937 rng(rng_seed);
    std::discrete_distribution<int> priors_dist(model.priors.begin(), model.priors.end());

    for (int ex = 0; ex < N_examples; ++ex)
    {
        // --- Fit check ---
        if (cursor.x + block_size.x > grid_size.x || cursor.y + block_size.y > grid_size.y ||
            cursor.z + block_size.z > grid_size.z)
        {
            cursor.x = 0;
            cursor.y += block_size.y + gap_y;

            if (cursor.y + block_size.y > grid_size.y)
            {
                cursor.y = 0;
                cursor.z += block_size.z + gap_z;
            }
            if (cursor.z + block_size.z > grid_size.z)
                break; // out of space
        }

        // --- Pick random base pattern a ---
        int a = priors_dist(rng);

        // Pick random direction
        std::uniform_int_distribution<int> dir_dist(0, ngh.get_K() - 1);
        int k = dir_dist(rng);
        Vector3i dir_offset = ngh.offsets()[k];

        // Pick random compatible neighbor b
        const auto &mask = model.compat[k][a];
        std::vector<int> allowed;
        allowed.reserve(P);
        for (int b_id = 0; b_id < P; ++b_id)
        {
            if (model.is_compatible(k, a, b_id))
            {
                allowed.push_back(b_id);
            }
        }
        if (allowed.empty())
            continue; // no neighbor possible in this dir
        std::uniform_int_distribution<size_t> bdist(0, allowed.size() - 1);
        int b = allowed[bdist(rng)];

        // --- Place pattern a ---
        for (int slot = 0; slot < ngh.get_K() + 1; ++slot)
        {
            Vector3i local = (slot == 0) ? Vector3i(0, 0, 0) : ngh.offsets()[slot - 1];
            Vector3i pos_world = cursor + (local + Vector3i(1, 1, 1)); // center offset in block
            int idx = properties.posToVoxelIndex(pos_world);
            if (idx >= 0 && idx < (int)result_voxels.size())
                result_voxels[idx] = model.patterns[a].voxels[slot];
        }

        // --- Place pattern b relative to a in dir k ---
        Vector3i b_origin = cursor + (dir_offset * 1) + Vector3i(1, 1, 1);
        for (int slot = 0; slot < ngh.get_K() + 1; ++slot)
        {
            Vector3i local = (slot == 0) ? Vector3i(0, 0, 0) : ngh.offsets()[slot - 1];
            Vector3i pos_world = b_origin + local;
            int idx = properties.posToVoxelIndex(pos_world);
            if (idx >= 0 && idx < (int)result_voxels.size())
                result_voxels[idx] = model.patterns[b].voxels[slot];
        }

        // Advance cursor in X
        cursor.x += block_size.x + gap_x;
        if (cursor.x + block_size.x > grid_size.x)
        {
            cursor.x = 0;
            cursor.y += block_size.y + gap_y;
            if (cursor.y + block_size.y > grid_size.y)
            {
                cursor.y = 0;
                cursor.z += block_size.z + gap_z;
            }
        }
    }

    voxel_world_rids.set_voxel_data(result_voxels);
}

struct CompatMismatch
{
    int k, a, b;
    bool expected, got;
};

std::vector<CompatMismatch> validate_compat(const PatternModel &model)
{
    std::vector<CompatMismatch> mismatches;
    int D = model.D;
    int P = static_cast<int>(model.patterns.size());
    for (int d = 0; d < D; ++d)
    {
        Vector3i delta = model.ngh->center_deltas()[d];
        for (int a = 0; a < P; ++a)
        {
            for (int b = 0; b < P; ++b)
            {
                bool expected = patterns_compatible_delta(*model.ngh, model.patterns[a], model.patterns[b], delta);
                bool got = model.is_compatible(d, a, b);
                if (expected != got)
                    mismatches.push_back({d, a, b, expected, got});
            }
        }
    }
    return mismatches;
}

// -------------------- Generator --------------------

void VoxelWorldWFCPatternGenerator::generate(RenderingDevice *rd, VoxelWorldRIDs &voxel_world_rids,
                                             const VoxelWorldProperties &properties)
{
    if (voxel_data.is_null())
    {
        ERR_PRINT("Voxel data is not set for the WFC pattern generator.");
        return;
    }
    voxel_data->load();

    // Config
    Moore neighborhood(2, true);
    // Face6Neighborhood neighborhood;
    const Neighborhood &ngh = neighborhood;

    // Template extraction
    const Vector3i template_size = voxel_data->get_size();
    PatternModel model = build_pattern_model_from_neighborhood(voxel_data, template_size, ngh);
    const int P = static_cast<int>(model.patterns.size());
    if (P == 0)
    {
        ERR_PRINT("PatternWFC: No patterns to work with.");
        return;
    }

    // debug_place_and_print_patterns(model, ngh, voxel_world_rids, properties);
    // UtilityFunctions::print("PatternWFC: Amount of mismatches between compat and exhaustive check: ",
    // validate_compat(model).size()); debug_place_pattern_pairs(model, ngh, voxel_world_rids, properties, 10000,
    // Time::get_singleton()->get_unix_time_from_system()); 
    // return;

    // Output grid size
    Vector3i grid_size(properties.grid_size.x, properties.grid_size.y, properties.grid_size.z);
    grid_size = grid_size.min(target_grid_size);

    const int N = grid_size.x * grid_size.y * grid_size.z;

    // Grid cells
    std::vector<std::unique_ptr<PatternCell>> grid;
    grid.reserve(N);
    for (int i = 0; i < N; ++i)
        grid.emplace_back(std::make_unique<EmptyCell>());

    auto pos_from_index = [&](int idx) -> Vector3i {
        int x = idx % grid_size.x;
        int y = (idx / grid_size.x) % grid_size.y;
        int z = idx / (grid_size.x * grid_size.y);
        return Vector3i(x, y, z);
    };

    auto collapse_to_pattern = [&](int idx, int pattern_id, bool is_debug = false) {
        auto cv = std::make_unique<CollapsedCell>();
        cv->pattern_id = pattern_id;
        cv->is_debug = is_debug;
        grid[idx] = std::move(cv);
    };

    // Initialize RNG
    std::mt19937 rng(Time::get_singleton()->get_unix_time_from_system());

    // Min-heap for selecting next collapse (lowest entropy first)
    struct HeapNode {
        float entropy;
        int index;
        uint32_t version;
        uint64_t tick;
    };

    struct HeapCompare {
        bool operator()(const HeapNode &a, const HeapNode &b) const {
            if (a.entropy != b.entropy)
                return a.entropy > b.entropy; // min-heap on entropy
            return a.tick < b.tick; // larger tick = more recent
        }
    };

    std::priority_queue<HeapNode, std::vector<HeapNode>, HeapCompare> heap;

    // setup superposition cells
    normalize(model.priors); // ensure priors sum to 1.0
    float priors_entropy = shannon_entropy(model.priors);

    auto init_superposition_from_priors = [&](int idx) -> SuperpositionCell * {
        auto sp = std::make_unique<SuperpositionCell>();
        sp->p = model.priors; // copy
        sp->domain_bits.resize((P + 63) / 64, ~0);
        sp->entropy = priors_entropy;
        sp->version = 0;
        auto *raw = sp.get();
        grid[idx] = std::move(sp);
        return raw;
    };

    auto apply_compat = [&](SuperpositionCell &tgt, int k, int neighbor_pattern_id) -> bool {
        const auto &mask = model.compat[k][neighbor_pattern_id];
        bool any = false;
        size_t nbits = model.priors.size();
        size_t nwords = (nbits + 63) / 64; // rounded up for tail

        for (size_t w = 0; w < nwords; ++w)
        {
            uint64_t before = tgt.domain_bits[w];
            uint64_t after = before & mask[w];

            // If this is the last word and there’s a partial tail, zero out high bits
            if (w == nwords - 1 && (nbits & 63))
            {
                uint64_t tailmask = (~0ULL) >> (64 - (nbits & 63));
                after &= tailmask;
            }

            if (after != before)
            {
                any = true;
                size_t base = w * 64;
                for (int i = 0; i < 64; ++i)
                {
                    size_t idx = base + i;
                    if (idx >= nbits)
                        break; // handles tail naturally
                    if ((after & (1ULL << i)))
                        tgt.p[idx] = model.priors[idx]; // reset to prior
                    else
                        tgt.p[idx] = 0.0f;
                }
            }

            tgt.domain_bits[w] = after;
        }

        if (normalize(tgt.p) <= 0.0f)
        {
            tgt.version = (~0u) - 1;
            return true;
        }

        if (!any)
            return false;

        tgt.entropy = shannon_entropy(tgt.p);
        return true;
    };

    auto init_from_single_neighbor = [&](int idx, int cond_index, int neighbor_pattern_id) -> SuperpositionCell * {
        auto *sp = init_superposition_from_priors(idx);
        if (!sp)
            return nullptr;
        apply_compat(*sp, cond_index, neighbor_pattern_id);
        if (sp->version == ~0u)
        {
            grid[idx].reset();
            return nullptr;
        }          // true contradiction
        return sp; // keep the cell even if no bits were removed
    };

    uint64_t global_tick = 0;

    // Seed: choose center-ish cell, init from priors
    int seed_idx = index_3d(Vector3i(grid_size.x / 2, grid_size.y / 2, grid_size.z / 2), grid_size);
    if (seed_idx < 0 || seed_idx >= N)
        seed_idx = 0;
    SuperpositionCell *seed_sp = init_superposition_from_priors(seed_idx);
    if (seed_sp)
        heap.push(HeapNode{seed_sp->entropy, seed_idx, seed_sp->version, global_tick++});

    int simple_timer_lol = 0;
    // Main loop
    while (!heap.empty())// && simple_timer_lol++ < 10000) // limit iterations to prevent infinite loops
    {
        HeapNode node = heap.top();
        heap.pop();
        if (grid[node.index]->kind() != PatternCell::Kind::SUPERPOSITION)
            continue;
        auto *sp = static_cast<SuperpositionCell *>(grid[node.index].get());
        if (sp->version != node.version)
            continue; // stale

        // Collapse
        int pat_id = weighted_sample(sp->p, rng);
        bool invalid = sp->version == ~0;
        collapse_to_pattern(node.index, pat_id, invalid);
        if (invalid)
            continue;

        // Propagate to neighbors
        Vector3i pos = pos_from_index(node.index);
        // const auto &offs = ngh.offsets();
        const auto &deltas = ngh.center_deltas();
        const int D = deltas.size();
        for (int d = 0; d < D; ++d)
        {
            Vector3i np = pos + deltas[d];
            if (!in_bounds(np, grid_size))
                continue;
            int nidx = index_3d(np, grid_size);

            auto kind = grid[nidx]->kind();
            if (kind == PatternCell::Kind::EMPTY)
            {
                if (auto *tgt = init_from_single_neighbor(nidx, d, pat_id))
                {
                    heap.push({tgt->entropy, nidx, tgt->version, global_tick++});
                }
            }
            else if (kind == PatternCell::Kind::SUPERPOSITION)
            {
                auto *tgt = static_cast<SuperpositionCell *>(grid[nidx].get());
                bool changed = apply_compat(*tgt, d, pat_id);
                if (changed)
                {
                    tgt->version += 1;
                    heap.push({tgt->entropy, nidx, tgt->version, global_tick++});
                }
            }
        }
    }

    // Convert to output voxels (center voxel of each collapsed pattern)
    std::vector<Voxel> result_voxels(voxel_world_rids.voxel_count, Voxel::create_air_voxel());

    for (int i = 0; i < N; ++i)
    {
        int result_idx = properties.posToVoxelIndex(pos_from_index(i));
        if (result_idx < 0 || result_idx >= static_cast<int>(result_voxels.size()))
            continue;

        if (grid[i]->kind() == PatternCell::Kind::COLLAPSED)
        {
            auto *cv = static_cast<CollapsedCell *>(grid[i].get());
            int pid = cv->pattern_id;
            if (cv->is_debug)
            {
                result_voxels[result_idx] = Voxel::create_voxel(Voxel::VOXEL_TYPE_SOLID, Color(1.0f, 0.0f, 1.0f));
            }
            else if (pid >= 0 && pid < P)
            {
                result_voxels[result_idx] = model.patterns[pid].voxels[0];
            }
        }
    }

    voxel_world_rids.set_voxel_data(result_voxels);
}
