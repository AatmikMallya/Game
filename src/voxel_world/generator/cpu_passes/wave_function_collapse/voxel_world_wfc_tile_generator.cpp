#include "voxel_world_wfc_tile_generator.h"
#include "bit_logic.h"
#include <array>
#include <cmath>
#include <godot_cpp/classes/time.hpp>
#include <limits>
#include <memory>
#include <queue>
#include <random>

using namespace godot;

// ---------------------------------------------------------------
// Utilities and internal structures
// ---------------------------------------------------------------

namespace
{

// Directions in 6-neighborhood
enum Dir
{
    POS_X = 0,
    NEG_X = 1,
    POS_Y = 2,
    NEG_Y = 3,
    POS_Z = 4,
    NEG_Z = 5
};

inline int opposite_dir(int d)
{
    static const int opp[6] = {NEG_X, POS_X, NEG_Y, POS_Y, NEG_Z, POS_Z};
    return opp[d];
}

// Oriented coords -> original coords (Y-rotation, optional horizontal flip)
inline Vector3i oriented_to_original(Vector3i in, Vector3i tile_size, int rot, bool flip)
{
    if (flip)
        in.x = tile_size.x - 1 - in.x;

    int x, y = in.y, z;
    switch (rot & 3)
    {
    case 0:
        x = in.x;
        z = in.z;
        break;
    case 1:
        x = in.z;
        z = tile_size.x - 1 - in.x;
        break;
    case 2:
        x = tile_size.x - 1 - in.x;
        z = tile_size.z - 1 - in.z;
        break;
    case 3:
        x = tile_size.z - 1 - in.z;
        z = in.x;
        break;
    default:
        x = in.x;
        z = in.z;
        break;
    }
    return {x, y, z};
}

struct FaceSig
{
    std::vector<Voxel> data;
    bool equals(const FaceSig &o) const
    {
        if (data.size() != o.data.size())
            return false;
        for (size_t i = 0; i < data.size(); ++i)
        {
            if (!(data[i] == o.data[i]))
                return false;
        }
        return true;
    }
};

inline FaceSig extract_face_sig(const Ref<VoxelData> &vd, Vector3i base_size, int rot, bool flip, int dir)
{
    FaceSig sig;

    Vector3i oriented_size = {(rot % 2 == 0) ? base_size.x : base_size.z, base_size.y,
                              (rot % 2 == 0) ? base_size.z : base_size.x};

    auto push_voxel = [&](Vector3i oriented_pos) {
        Vector3i orig = oriented_to_original(oriented_pos, base_size, rot, flip);
        sig.data.push_back(vd->get_voxel_at(orig));
    };

    if (dir == POS_X || dir == NEG_X)
    {
        int xo = (dir == POS_X) ? (oriented_size.x - 1) : 0;
        sig.data.reserve(size_t(base_size.y) * oriented_size.z);
        for (int y = 0; y < base_size.y; ++y)
            for (int zo = 0; zo < oriented_size.z; ++zo)
                push_voxel({xo, y, zo});
    }
    else if (dir == POS_Z || dir == NEG_Z)
    {
        int zo = (dir == POS_Z) ? (oriented_size.z - 1) : 0;
        sig.data.reserve(size_t(base_size.y) * oriented_size.x);
        for (int y = 0; y < base_size.y; ++y)
            for (int xo = 0; xo < oriented_size.x; ++xo)
                push_voxel({xo, y, zo});
    }
    else
    { // POS_Y or NEG_Y
        int y = (dir == POS_Y) ? (base_size.y - 1) : 0;
        sig.data.reserve(size_t(oriented_size.x) * oriented_size.z);
        for (int xo = 0; xo < oriented_size.x; ++xo)
            for (int zo = 0; zo < oriented_size.z; ++zo)
                push_voxel({xo, y, zo});
    }
    return sig;
}

inline Voxel sample_center_voxel(const Ref<VoxelData> &vd, Vector3i base_size, int rot, bool flip)
{
    Vector3i oriented_size = {(rot % 2 == 0) ? base_size.x : base_size.z, base_size.y,
                              (rot % 2 == 0) ? base_size.z : base_size.x};
    Vector3i oriented_center = {oriented_size.x / 2, base_size.y / 2, oriented_size.z / 2};
    Vector3i orig = oriented_to_original(oriented_center, base_size, rot, flip);
    return vd->get_voxel_at(orig);
}

// Grid indexing helpers (tile grid, not voxels)
inline int index_3d(const Vector3i &p, const Vector3i &size)
{
    return p.x + size.x * (p.z + size.z * p.y); // X-fastest, then Z, then Y
}
inline Vector3i pos_from_index(int idx, const Vector3i &size)
{
    int y = idx / (size.x * size.z);
    int rem = idx - y * size.x * size.z;
    int z = rem / size.x;
    int x = rem - z * size.x;
    return {x, y, z};
}
inline bool in_bounds(const Vector3i &p, const Vector3i &size)
{
    return p.x >= 0 && p.y >= 0 && p.z >= 0 && p.x < size.x && p.y < size.y && p.z < size.z;
}

static const Vector3i kDirDelta[6] = {{+1, 0, 0}, {-1, 0, 0}, {0, +1, 0}, {0, -1, 0}, {0, 0, +1}, {0, 0, -1}};

// WFC model and cells
struct Pattern
{
    int tile_idx = -1;
    int rot = 0;       // 0..3
    bool flip = false; // horizontal mirror
    float weight = 1.0f;
    FaceSig faces[6];
    uint8_t solid_faces = 0; // bit i set if face i is non‑air

    Voxel center_voxel = Voxel::create_air_voxel();
};

struct TileModel
{
    Vector3i tile_size = {1, 1, 1};
    std::vector<Pattern> patterns; // size P
    std::vector<float> priors;     // size P (normalized)
    // compat[dir][neighbor_pattern] => bitmask (vector<uint32_t>) of allowed patterns in target
    std::array<std::vector<std::vector<uint32_t>>, 6> compat;
};

TileModel g_model;

// Normalize priors to sum to 1
inline void normalize(std::vector<float> &p)
{
    double s = 0.0;
    for (float v : p)
        s += std::max(0.0f, v);
    if (s <= 0.0)
    {
        float u = 1.0f / float(std::max<size_t>(size_t(1), p.size()));
        for (auto &v : p)
            v = u;
    }
    else
    {
        for (auto &v : p)
            v = float(v / s);
    }
}

// Shannon entropy over masked subset (domain_bits)
inline float shannon_entropy(const std::vector<float> &priors, const std::vector<uint32_t> &mask_bits)
{
    const size_t Pbits = priors.size();
    const size_t Pwords = (Pbits + 31) / 32;

    double W = 0.0;
    double WlogW = 0.0;

    for (size_t w = 0, base = 0; w < Pwords; ++w, base += 32)
    {
        uint32_t word = (w < mask_bits.size()) ? mask_bits[w] : 0u;
        while (word)
        {
            int b = ctz32(word);
            size_t p = base + size_t(b);
            if (p < Pbits)
            {
                double wp = std::max(1e-12, double(priors[p]));
                W += wp;
                WlogW += wp * std::log(wp);
            }
            word &= (word - 1);
        }
    }
    if (W <= 0.0)
        return 0.0f;
    return float(std::log(W) - (WlogW / W));
}

// Weighted sample from priors under mask
template <class URNG>
inline int weighted_sample_bits(const std::vector<float> &priors, const std::vector<uint32_t> &mask_bits, URNG &rng)
{
    const size_t Pbits = priors.size();
    const size_t Pwords = (Pbits + 31) / 32;

    double total = 0.0;
    for (size_t w = 0, base = 0; w < Pwords; ++w, base += 32)
    {
        uint32_t word = (w < mask_bits.size()) ? mask_bits[w] : 0u;
        while (word)
        {
            int b = ctz32(word);
            size_t p = base + size_t(b);
            if (p < Pbits)
                total += std::max(0.0f, priors[p]);
            word &= (word - 1);
        }
    }
    if (total <= 0.0)
        return -1;

    std::uniform_real_distribution<double> U(0.0, total);
    double r = U(rng);

    for (size_t w = 0, base = 0; w < Pwords; ++w, base += 32)
    {
        uint32_t word = (w < mask_bits.size()) ? mask_bits[w] : 0u;
        while (word)
        {
            int b = ctz32(word);
            size_t p = base + size_t(b);
            if (p < Pbits)
            {
                double wv = std::max(0.0f, priors[p]);
                if (r <= wv)
                    return int(p);
                r -= wv;
            }
            word &= (word - 1);
        }
    }
    return -1;
}

static bool is_all_air_face(const FaceSig &sig)
{
    for (int i = 0; i < sig.data.size(); ++i)
    {
        if (!sig.data[i].is_air())
            return false;
    }
    return true;
}

} // namespace

bool VoxelWorldWFCTileGenerator::prepare_voxel_tiles(const TypedArray<WaveFunctionCollapseTile> &tiles)
{
    const Vector3i init_size = Vector3i(-1, -1, -1);
    g_model.tile_size = init_size;

    g_model.patterns.clear();
    for (auto &vec : g_model.compat)
        vec.clear();
    g_model.priors.clear();

    if (tiles.is_empty())
    {
        ERR_PRINT("No tiles provided for WFC tile generator.");
        return false;
    }

    for (int i = 0; i < tiles.size(); ++i)
    {
        const Ref<WaveFunctionCollapseTile> tile = tiles[i];
        if (!tile.is_valid())
        {
            ERR_PRINT("Invalid tile in voxel tiles array.");
            return false;
        }
        if (tile->get_voxel_tile().is_null())
        {
            ERR_PRINT("Tile has no voxel data.");
            return false;
        }

        Error success = tile->get_voxel_tile()->load();

        if (success != OK)
        {
            ERR_PRINT("Failed to load voxel data for tile.");
            return false;
        }

        Vector3i sz = tile->get_voxel_tile()->get_size();
        if (g_model.tile_size != sz && g_model.tile_size != init_size)
        {
            ERR_PRINT("All tiles must have the same size.");
            return false;
        }

        g_model.tile_size = sz;
    }

    for (int i = 0; i < tiles.size(); ++i)
    {
        const Ref<WaveFunctionCollapseTile> tile = tiles[i];
        const Ref<VoxelData> vox = tile->get_voxel_tile();

        const bool can_rot = tile->get_can_rotate_y();
        const bool can_flip = tile->get_can_flip_horizontal();
        const int rot_count = can_rot ? 4 : 1;
        const int flip_count = can_flip ? 2 : 1;

        const float base_weight = tile->get_weight();
        const float orient_norm = Math::max(1.0f, float(rot_count * flip_count));

        for (int r = 0; r < rot_count; ++r)
        {
            for (int f = 0; f < flip_count; ++f)
            {
                Pattern p;
                p.tile_idx = i;
                p.rot = r;
                p.flip = (f != 0);
                p.weight = base_weight / orient_norm;

                p.faces[POS_X] = extract_face_sig(vox, g_model.tile_size, r, p.flip, POS_X);
                p.faces[NEG_X] = extract_face_sig(vox, g_model.tile_size, r, p.flip, NEG_X);
                p.faces[POS_Y] = extract_face_sig(vox, g_model.tile_size, r, p.flip, POS_Y);
                p.faces[NEG_Y] = extract_face_sig(vox, g_model.tile_size, r, p.flip, NEG_Y);
                p.faces[POS_Z] = extract_face_sig(vox, g_model.tile_size, r, p.flip, POS_Z);
                p.faces[NEG_Z] = extract_face_sig(vox, g_model.tile_size, r, p.flip, NEG_Z);

                p.center_voxel = sample_center_voxel(vox, g_model.tile_size, r, p.flip);
                g_model.patterns.push_back(std::move(p));
            }
        }
    }

    const size_t P = g_model.patterns.size();
    if (P == 0)
    {
        ERR_PRINT("No oriented patterns generated.");
        return false;
    }

    // Priors
    g_model.priors.resize(P);
    for (size_t p = 0; p < P; ++p)
        g_model.priors[p] = std::max(0.0f, g_model.patterns[p].weight);
    normalize(g_model.priors);

    // Build compatibility masks
    const size_t Pwords = (P + 31) / 32;
    for (int d = 0; d < 6; ++d)
    {
        g_model.compat[d].assign(P, std::vector<uint32_t>(Pwords, 0u));
    }

    for (size_t p = 0; p < P; ++p)
    {
        for (size_t q = 0; q < P; ++q)
        {
            for (int d = 0; d < 6; ++d)
            {
                int od = opposite_dir(d);
                // Skip if either face is all air
                // if (is_all_air_face(g_model.patterns[p].faces[d])) {
                //     continue;
                // }

                if (g_model.patterns[p].faces[d].equals(g_model.patterns[q].faces[od]))
                {
                    std::vector<uint32_t> &mask = g_model.compat[d][p];
                    size_t w = q >> 5;
                    uint32_t bit = 1u << (q & 31);
                    mask[w] |= bit;
                }
            }
        }
        for (int d = 0; d < 6; ++d)
        {
            auto &pat = g_model.patterns[p];
            if (!is_all_air_face(pat.faces[d]))
            {
                pat.solid_faces |= (1u << d);
            }
        }
    }

    return true;
}

// ---------------------------------------------------------------
// VoxelWorldWFCTileGenerator::generate
// ---------------------------------------------------------------

bool VoxelWorldWFCTileGenerator::generate(std::vector<Voxel> &result_voxels, const Vector3i bounds_min,
                                          const Vector3i bounds_max, const VoxelWorldProperties &properties)
{
    if (!prepare_voxel_tiles(voxel_tiles))
        return false;

    Vector3i bounds_size = bounds_max - bounds_min;

    Vector3i grid_size = target_grid_size.min(bounds_size);
    grid_size = (Vector3(grid_size) / (Vector3(g_model.tile_size) * voxel_scale)).floor();
    if (grid_size.x <= 0 || grid_size.y <= 0 || grid_size.z <= 0)
    {
        ERR_PRINT("Grid size too small for tile size.");
        return false;
    }

    const int N = grid_size.x * grid_size.y * grid_size.z;
    const size_t P = g_model.patterns.size();
    const size_t Pwords = (P + 31) / 32;

    // Cells
    struct PatternCell
    {
        enum class Kind
        {
            EMPTY,
            SUPERPOSITION,
            COLLAPSED
        };
        virtual ~PatternCell() = default;
        virtual Kind kind() const = 0;
    };
    struct EmptyCell final : PatternCell
    {
        Kind kind() const override
        {
            return Kind::EMPTY;
        }
    };
    struct SuperpositionCell final : PatternCell
    {
        std::vector<uint32_t> domain_bits; // bitset over patterns
        float entropy = 0.0f;
        uint32_t version = 0;
        bool initialized_by_nonair_connection = false;
        Kind kind() const override
        {
            return Kind::SUPERPOSITION;
        }
    };
    struct CollapsedCell final : PatternCell
    {
        int pattern_id = -1;
        bool is_debug = false;
        Kind kind() const override
        {
            return Kind::COLLAPSED;
        }
    };

    // RNG
    std::mt19937 rng(Time::get_singleton()->get_unix_time_from_system());

    // Priority queue (min-heap by entropy, with recency tiebreaker)
    struct HeapNode
    {
        float entropy;
        int index;
        uint32_t version;
        uint64_t tick;
    };
    struct HeapCompare
    {
        bool operator()(const HeapNode &a, const HeapNode &b) const
        {
            if (a.entropy != b.entropy)
                return a.entropy > b.entropy;
            return a.tick < b.tick;
        }
    };
    std::priority_queue<HeapNode, std::vector<HeapNode>, HeapCompare> heap;
    uint64_t global_tick = 1;

    // Grid init
    std::vector<std::unique_ptr<PatternCell>> grid(N);
    for (int i = 0; i < N; ++i)
        grid[i] = std::make_unique<EmptyCell>();

    // Precompute full-mask and initial entropy
    std::vector<uint32_t> full(Pwords, ~0u);
    if (P & 31)
    {
        uint32_t tailmask = (~0u) >> (32 - int(P & 31));
        full[Pwords - 1] = tailmask;
    }
    const float priors_entropy = shannon_entropy(g_model.priors, full);

    // Helpers

    // Direction bit order: POS_X, NEG_X, POS_Y, NEG_Y, POS_Z, NEG_Z
    uint8_t boundary_air_mask = (1 << POS_X) | (1 << NEG_X) | (1 << POS_Z) | (1 << NEG_Z);

    auto apply_boundary_constraints = [&](SuperpositionCell &sp, const Vector3i &pos) {
        for (int d = 0; d < 6; ++d)
        {
            if (!(boundary_air_mask & (1 << d)))
                continue; // not enforcing this face

            // Are we physically at the boundary in this dir?
            if ((d == POS_X && pos.x == grid_size.x - 1) || (d == NEG_X && pos.x == 0) ||
                (d == POS_Y && pos.y == grid_size.y - 1) || (d == NEG_Y && pos.y == 0) ||
                (d == POS_Z && pos.z == grid_size.z - 1) || (d == NEG_Z && pos.z == 0))
            {
                // Remove all patterns that do NOT have air on this face
                for (size_t p = 0; p < g_model.patterns.size(); ++p)
                {
                    size_t w = p >> 5;
                    uint32_t bit = 1u << (p & 31);
                    if (sp.domain_bits[w] & bit)
                    {
                        if (g_model.patterns[p].solid_faces & (1u << d))
                        {
                            sp.domain_bits[w] &= ~bit; // disallow solid touching boundary
                        }
                    }
                }
            }
        }
        sp.entropy = shannon_entropy(g_model.priors, sp.domain_bits);
    };

    auto init_superposition_from_priors = [&](int idx) -> SuperpositionCell * {
        auto sp = std::make_unique<SuperpositionCell>();
        sp->domain_bits = full; // already tail-masked
        sp->entropy = priors_entropy;
        sp->version = 0;

        Vector3i pos = pos_from_index(idx, grid_size);
        apply_boundary_constraints(*sp, pos); // NEW

        auto *raw = sp.get();
        grid[idx] = std::move(sp);
        return raw;
    };

    auto apply_constraints = [&](SuperpositionCell &tgt, int dir, int neighbor_pattern_id) -> bool {
        const std::vector<uint32_t> &mask = g_model.compat[dir][size_t(neighbor_pattern_id)];
        bool any_changed = false;
        bool all_zero = true;

        if ((g_model.patterns[neighbor_pattern_id].solid_faces >> dir) & 1)
            tgt.initialized_by_nonair_connection = true;

        if (tgt.domain_bits.size() < Pwords)
            tgt.domain_bits.resize(Pwords, 0u);

        for (size_t w = 0; w < Pwords; ++w)
        {
            uint32_t before = tgt.domain_bits[w];
            uint32_t src = (w < mask.size()) ? mask[w] : 0u;
            uint32_t after = before & src;

            if (w == Pwords - 1 && (P & 31))
            {
                uint32_t tailmask = (~0u) >> (32 - int(P & 31));
                after &= tailmask;
            }

            if (after != before)
                any_changed = true;
            if (after)
                all_zero = false;
            tgt.domain_bits[w] = after;
        }

        if (all_zero)
        {
            tgt.version = ~0u; // contradiction
            return true;
        }

        if (!any_changed)
            return false;

        tgt.entropy = shannon_entropy(g_model.priors, tgt.domain_bits);
        return true;
    };

    auto init_from_single_neighbor = [&](int idx, int dir, int neighbor_pattern_id) -> PatternCell * {
        auto *sp = init_superposition_from_priors(idx);
        if (!sp)
            return nullptr;
        (void)apply_constraints(*sp, dir, neighbor_pattern_id);
        if (sp->version == ~0u)
        {
            auto cl = std::make_unique<CollapsedCell>();
            cl->pattern_id = 0;
            cl->is_debug = true;
            auto *raw = cl.get();
            grid[idx] = std::move(cl);
            return raw;
        }
        return sp;
    };

    auto collapse_to_pattern = [&](int idx, int pattern_id, bool invalid) {
        auto cl = std::make_unique<CollapsedCell>();
        cl->pattern_id = invalid ? -1 : pattern_id;
        cl->is_debug = invalid || (pattern_id < 0);
        grid[idx] = std::move(cl);
    };

    auto push_sp = [&](int idx, SuperpositionCell *sp) {
        heap.push(HeapNode{sp->entropy, idx, sp->version, global_tick++});
    };

    // Seed at grid center (fallback to 0)
    {
        int seed_idx = index_3d((Vector3(grid_size) * seed_position_normalized).floor(), grid_size);
        if (seed_idx < 0 || seed_idx >= N)
            seed_idx = 0;
        if (auto *seed_sp = init_superposition_from_priors(seed_idx))
        {
            seed_sp->initialized_by_nonair_connection = true;
            push_sp(seed_idx, seed_sp);
        }
    }

    // ---------------------- Single while-loop WFC ----------------------
    while (!heap.empty())
    {
        HeapNode node = heap.top();
        heap.pop();

        if (!grid[node.index])
            continue;
        if (grid[node.index]->kind() != PatternCell::Kind::SUPERPOSITION)
            continue;

        auto *sp = static_cast<SuperpositionCell *>(grid[node.index].get());
        if (sp->version != node.version)
            continue; // stale

        if (!(sp->initialized_by_nonair_connection || !need_nonair_connection))
            continue;

        // Collapse current cell
        int pat_id = weighted_sample_bits(g_model.priors, sp->domain_bits, rng);
        bool invalid = (sp->version == ~0u) || (pat_id < 0);
        collapse_to_pattern(node.index, pat_id, invalid);
        if (invalid)
            continue;

        // Propagate to 6 neighbors
        Vector3i pos = pos_from_index(node.index, grid_size);
        for (int d = 0; d < 6; ++d)
        {
            Vector3i np = {pos.x + kDirDelta[d].x, pos.y + kDirDelta[d].y, pos.z + kDirDelta[d].z};
            if (!in_bounds(np, grid_size))
                continue;

            int nidx = index_3d(np, grid_size);
            auto kind = grid[nidx]->kind();

            if (kind == PatternCell::Kind::EMPTY)
            {
                if (auto *tgt = init_from_single_neighbor(nidx, d, pat_id))
                {
                    if (tgt->kind() == PatternCell::Kind::SUPERPOSITION)
                        push_sp(nidx, static_cast<SuperpositionCell *>(tgt));
                }
            }
            else if (kind == PatternCell::Kind::SUPERPOSITION)
            {
                auto *tgt = static_cast<SuperpositionCell *>(grid[nidx].get());
                if (apply_constraints(*tgt, d, pat_id))
                {
                    if (tgt->version != ~0u)
                    {
                        // changed and still valid: bump version and push
                        tgt->version += 1;
                        push_sp(nidx, tgt);
                    }
                    else
                    {
                        // contradiction: collapse to debug immediately
                        collapse_to_pattern(nidx, -1, true);
                    }
                }
            }
        }
    }

    for (int i = 0; i < N; ++i)
    {
        if (!grid[i] || grid[i]->kind() != PatternCell::Kind::COLLAPSED)
            continue;

        auto *cv = static_cast<CollapsedCell *>(grid[i].get());
        Vector3i cell = pos_from_index(i, grid_size);

        // Guard BEFORE indexing patterns
        if (cv->is_debug || cv->pattern_id < 0 || size_t(cv->pattern_id) >= P)
        {
            if (show_contradictions)
            {
                // write magenta voxel(s)
            }
            continue;
        }

        const Pattern &pat = g_model.patterns[size_t(cv->pattern_id)];
        if (pat.tile_idx < 0 || pat.tile_idx >= voxel_tiles.size())
            continue;

        const Ref<WaveFunctionCollapseTile> tile = voxel_tiles[pat.tile_idx];
        if (tile.is_null() || tile->get_voxel_tile().is_null())
            continue;

        const Ref<VoxelData> vox = tile->get_voxel_tile();

        Vector3i scaled_tile_size = {int(std::round(g_model.tile_size.x * voxel_scale)),
                                     int(std::round(g_model.tile_size.y * voxel_scale)),
                                     int(std::round(g_model.tile_size.z * voxel_scale))};

        for (int lx = 0; lx < scaled_tile_size.x; ++lx)
            for (int ly = 0; ly < scaled_tile_size.y; ++ly)
                for (int lz = 0; lz < scaled_tile_size.z; ++lz)
                {
                    int src_lx = std::min(int(g_model.tile_size.x - 1), int(std::floor(lx / voxel_scale)));
                    int src_ly = std::min(int(g_model.tile_size.y - 1), int(std::floor(ly / voxel_scale)));
                    int src_lz = std::min(int(g_model.tile_size.z - 1), int(std::floor(lz / voxel_scale)));

                    Vector3i world = {cell.x * scaled_tile_size.x + lx, cell.y * scaled_tile_size.y + ly,
                                      cell.z * scaled_tile_size.z + lz};

                    int out_idx = properties.pos_to_voxel_index(world + bounds_min);
                    if (out_idx < 0 || out_idx >= static_cast<int>(result_voxels.size()))
                        continue;

                    if (only_replace_air && !result_voxels[out_idx].is_air())
                        continue;

                    Vector3i src = oriented_to_original({src_lx, src_ly, src_lz}, g_model.tile_size, pat.rot, pat.flip);
                    Voxel v = vox->get_voxel_at(src);
                    if (do_not_place_air && v.is_air())
                        continue;

                    Color c = v.get_color();
                    int type = v.get_type();
                    if (add_color_noise)
                        c = Utils::randomized_color(c);
                    result_voxels[out_idx] = Voxel::create_voxel(type, c);
                }
    }

    return true;

    // voxel_world_rids.set_voxel_data(result_voxels);
}
