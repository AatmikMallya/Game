#include "voxel_data_vox.h"

namespace {
struct VoxXYZI {
    uint8_t x, y, z, i;
};

static inline Color palette_entry_to_color(uint32_t abgr) {
    float r = ((abgr >> 0) & 0xFF) / 255.0f;
    float g = ((abgr >> 8) & 0xFF) / 255.0f;
    float b = ((abgr >> 16) & 0xFF) / 255.0f;
    float a = ((abgr >> 24) & 0xFF) / 255.0f;
    return Color(r, g, b, a);
}

// 
// 
}

// Default MagicaVoxel palette (ABGR). Omitted here for brevity—include the 256-entry array in your source.
const uint32_t VoxelDataVox::DEFAULT_VOX_PALETTE_ABGR[256] = {
    0x00000000, 0xffffffff, 0xffccffff, 0xff99ffff, 0xff66ffff, 0xff33ffff, 0xff00ffff, 0xffffccff, 0xffccccff, 0xff99ccff, 0xff66ccff, 0xff33ccff, 0xff00ccff, 0xffff99ff, 0xffcc99ff, 0xff9999ff,
	0xff6699ff, 0xff3399ff, 0xff0099ff, 0xffff66ff, 0xffcc66ff, 0xff9966ff, 0xff6666ff, 0xff3366ff, 0xff0066ff, 0xffff33ff, 0xffcc33ff, 0xff9933ff, 0xff6633ff, 0xff3333ff, 0xff0033ff, 0xffff00ff,
	0xffcc00ff, 0xff9900ff, 0xff6600ff, 0xff3300ff, 0xff0000ff, 0xffffffcc, 0xffccffcc, 0xff99ffcc, 0xff66ffcc, 0xff33ffcc, 0xff00ffcc, 0xffffcccc, 0xffcccccc, 0xff99cccc, 0xff66cccc, 0xff33cccc,
	0xff00cccc, 0xffff99cc, 0xffcc99cc, 0xff9999cc, 0xff6699cc, 0xff3399cc, 0xff0099cc, 0xffff66cc, 0xffcc66cc, 0xff9966cc, 0xff6666cc, 0xff3366cc, 0xff0066cc, 0xffff33cc, 0xffcc33cc, 0xff9933cc,
	0xff6633cc, 0xff3333cc, 0xff0033cc, 0xffff00cc, 0xffcc00cc, 0xff9900cc, 0xff6600cc, 0xff3300cc, 0xff0000cc, 0xffffff99, 0xffccff99, 0xff99ff99, 0xff66ff99, 0xff33ff99, 0xff00ff99, 0xffffcc99,
	0xffcccc99, 0xff99cc99, 0xff66cc99, 0xff33cc99, 0xff00cc99, 0xffff9999, 0xffcc9999, 0xff999999, 0xff669999, 0xff339999, 0xff009999, 0xffff6699, 0xffcc6699, 0xff996699, 0xff666699, 0xff336699,
	0xff006699, 0xffff3399, 0xffcc3399, 0xff993399, 0xff663399, 0xff333399, 0xff003399, 0xffff0099, 0xffcc0099, 0xff990099, 0xff660099, 0xff330099, 0xff000099, 0xffffff66, 0xffccff66, 0xff99ff66,
	0xff66ff66, 0xff33ff66, 0xff00ff66, 0xffffcc66, 0xffcccc66, 0xff99cc66, 0xff66cc66, 0xff33cc66, 0xff00cc66, 0xffff9966, 0xffcc9966, 0xff999966, 0xff669966, 0xff339966, 0xff009966, 0xffff6666,
	0xffcc6666, 0xff996666, 0xff666666, 0xff336666, 0xff006666, 0xffff3366, 0xffcc3366, 0xff993366, 0xff663366, 0xff333366, 0xff003366, 0xffff0066, 0xffcc0066, 0xff990066, 0xff660066, 0xff330066,
	0xff000066, 0xffffff33, 0xffccff33, 0xff99ff33, 0xff66ff33, 0xff33ff33, 0xff00ff33, 0xffffcc33, 0xffcccc33, 0xff99cc33, 0xff66cc33, 0xff33cc33, 0xff00cc33, 0xffff9933, 0xffcc9933, 0xff999933,
	0xff669933, 0xff339933, 0xff009933, 0xffff6633, 0xffcc6633, 0xff996633, 0xff666633, 0xff336633, 0xff006633, 0xffff3333, 0xffcc3333, 0xff993333, 0xff663333, 0xff333333, 0xff003333, 0xffff0033,
	0xffcc0033, 0xff990033, 0xff660033, 0xff330033, 0xff000033, 0xffffff00, 0xffccff00, 0xff99ff00, 0xff66ff00, 0xff33ff00, 0xff00ff00, 0xffffcc00, 0xffcccc00, 0xff99cc00, 0xff66cc00, 0xff33cc00,
	0xff00cc00, 0xffff9900, 0xffcc9900, 0xff999900, 0xff669900, 0xff339900, 0xff009900, 0xffff6600, 0xffcc6600, 0xff996600, 0xff666600, 0xff336600, 0xff006600, 0xffff3300, 0xffcc3300, 0xff993300,
	0xff663300, 0xff333300, 0xff003300, 0xffff0000, 0xffcc0000, 0xff990000, 0xff660000, 0xff330000, 0xff0000ee, 0xff0000dd, 0xff0000bb, 0xff0000aa, 0xff000088, 0xff000077, 0xff000055, 0xff000044,
	0xff000022, 0xff000011, 0xff00ee00, 0xff00dd00, 0xff00bb00, 0xff00aa00, 0xff008800, 0xff007700, 0xff005500, 0xff004400, 0xff002200, 0xff001100, 0xffee0000, 0xffdd0000, 0xffbb0000, 0xffaa0000,
	0xff880000, 0xff770000, 0xff550000, 0xff440000, 0xff220000, 0xff110000, 0xffeeeeee, 0xffdddddd, 0xffbbbbbb, 0xffaaaaaa, 0xff888888, 0xff777777, 0xff555555, 0xff444444, 0xff222222, 0xff111111
};

Error VoxelDataVox::load() {
    palette.clear();
    voxels.clear();
    size = Vector3i();

    if (file_path.is_empty()) {
        UtilityFunctions::printerr("VoxelDataVox: file_path is empty");
        return ERR_INVALID_PARAMETER;
    }

    Ref<FileAccess> f = FileAccess::open(file_path, FileAccess::READ);
    if (f.is_null()) {
        UtilityFunctions::printerr("VoxelDataVox: cannot open: ", file_path);
        return ERR_CANT_OPEN;
    }

    auto read_u32 = [&]() -> uint32_t { return f->get_32(); };
    auto read_tag = [&]() -> uint32_t { return f->get_32(); };

    uint32_t magic = read_tag();
    if (magic != String("VOX ").hash()) {
        // Read literal 'VOX ' bytes instead, since FileAccess returns numbers
        f->seek(0);
        PackedByteArray hdr = f->get_buffer(4);
        if (hdr.size() != 4 || hdr[0] != 'V' || hdr[1] != 'O' || hdr[2] != 'X' || hdr[3] != ' ') {
            UtilityFunctions::printerr("VoxelDataVox: invalid header");
            return ERR_FILE_CORRUPT;
        }
    }
    uint32_t version = read_u32(); // usually 150

    uint32_t main_id = read_tag();    // 'MAIN'
    uint32_t main_size = read_u32();  // 0
    uint32_t main_child = read_u32(); // size of all children

    Vector3i found_size(0, 0, 0);
    std::vector<VoxXYZI> points;
    std::array<uint32_t, 256> pal_abgr{};
    bool has_custom_palette = false;
    for (int i = 0; i < 256; ++i) pal_abgr[i] = DEFAULT_VOX_PALETTE_ABGR[i];

    uint64_t end_of_main = f->get_position() + main_child;

    while (f->get_position() < end_of_main && !f->eof_reached()) {
        PackedByteArray tag_b = f->get_buffer(4);
        if (tag_b.size() < 4) break;
        uint32_t id = (uint32_t)tag_b[0] | ((uint32_t)tag_b[1] << 8) | ((uint32_t)tag_b[2] << 16) | ((uint32_t)tag_b[3] << 24);
        uint32_t chunk_size = read_u32();
        uint32_t child_size = read_u32();
        uint64_t chunk_start = f->get_position();

        if (tag_b[0]=='S'&&tag_b[1]=='I'&&tag_b[2]=='Z'&&tag_b[3]=='E') {
            int32_t sx = (int32_t)read_u32();
            int32_t sy = (int32_t)read_u32();
            int32_t sz = (int32_t)read_u32();
            found_size = Vector3i(sx, sy, sz);
        } else if (tag_b[0]=='X'&&tag_b[1]=='Y'&&tag_b[2]=='Z'&&tag_b[3]=='I') {
            uint32_t n = read_u32();
            points.resize(n);
            PackedByteArray buf = f->get_buffer(n * 4);
            for (uint32_t i = 0; i < n; ++i) {
                points[i].x = buf[i * 4 + 0];
                points[i].y = buf[i * 4 + 1];
                points[i].z = buf[i * 4 + 2];
                points[i].i = buf[i * 4 + 3];
            }
        } else if (tag_b[0]=='R'&&tag_b[1]=='G'&&tag_b[2]=='B'&&tag_b[3]=='A') {
            PackedByteArray buf = f->get_buffer(256 * 4);
            if (buf.size() == 256 * 4) {
                for (int i = 0; i < 256; ++i) {
                    uint32_t abgr = (uint32_t)buf[i * 4 + 0]
                                  | ((uint32_t)buf[i * 4 + 1] << 8)
                                  | ((uint32_t)buf[i * 4 + 2] << 16)
                                  | ((uint32_t)buf[i * 4 + 3] << 24);
                    pal_abgr[i] = abgr;
                }
                has_custom_palette = true;
            }
        } else {
            f->seek(chunk_start + chunk_size);
        }

        f->seek(chunk_start + chunk_size + child_size);
    }

    if (found_size == Vector3i()) {
        UtilityFunctions::printerr("VoxelDataVox: missing SIZE chunk");
        return ERR_FILE_CORRUPT;
    }

    size = found_size;
    palette.resize(256);
    for (int i = 0; i < 256; ++i) palette[i] = palette_entry_to_color(pal_abgr[i]);

    voxels.assign((size_t)size.x * size.y * size.z, Voxel::create_air_voxel());

    auto index3 = [&](int x, int y, int z) -> size_t {
        return ((size_t)z * size.y + (size_t)y) * size.x + (size_t)x;
    };

    for (const auto &p : points) {
        if (p.x >= size.x || p.y >= size.y || p.z >= size.z || p.i <= 0) continue;
        uint8_t pal_index = p.i;
        Color c = palette[pal_index ? pal_index - 1 : 0];
        Voxel v = Voxel::create_voxel(Voxel::VOXEL_TYPE_SOLID, c);
        voxels[index3(p.x, p.y, p.z)] = v;
    }

    return OK;
}
