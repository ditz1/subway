#include <terrain.hpp>
#include <stage.hpp>
#include <cstring>

#if defined(PLATFORM_WEB)
    #define GLSL_VERSION 100
#else
    #define GLSL_VERSION 330
#endif

static const int half_width = 96;        // columns meshed either side of the piece centre line
static const int gate_run = 8;           // cells of straight, level track at each end of a piece
static const float bed_radius = 2.0f;    // flat ground under the track
static const float carve_radius = 8.0f;  // cuttings/embankments fade out by here, must be <= gate_run
static const float tree_clearance = 5.0f;
static const float corner_radius = 1.5f; // tightest bend that fits the gauge, spans the corner block and its neighbours
static const int min_run = 3;            // straight blocks between corners, so bends never overlap
static const float tree_clearance_near = 9.0f; // on the +x side, where the camera sits, so the train stays in view
static const float rail_height = 0.3f;

static unsigned char perm[512];
static unsigned int terrain_seed = 0;

static Shader terrain_shader;
static Shader water_shader;
static Shader depth_shader;
static RenderTexture2D shadow_target;
static bool shadow_pass = false;
static const int shadow_resolution = 2048;
static const float shadow_extent = 80.0f; // half width of the area around the train that gets shadows
static Vector3 sun_dir;
static int terrain_light_loc, depth_light_loc;
static int terrain_view_loc, water_view_loc, water_time_loc;
static Color sky_color = {150, 196, 232, 255};

enum Block { GRAVEL, BALLAST, SAND, SEABED, GRASS, STONE, SNOW };

// ---------------------------------------------------------------- noise

static float Mix(float a, float b, float t) { return a + (b - a) * t; }

static float SmoothStep(float lo, float hi, float x) {
    float t = Clamp((x - lo) / (hi - lo), 0.0f, 1.0f);
    return t * t * (3.0f - 2.0f * t);
}

static float Perlin2D(float x, float y) {
    auto fade = [](float t) { return t * t * t * (t * (t * 6 - 15) + 10); };
    auto grad = [](int hash, float x, float y) {
        int h = hash & 15;
        float u = h < 8 ? x : y;
        float v = h < 4 ? y : h == 12 || h == 14 ? x : 0;
        return ((h & 1) == 0 ? u : -u) + ((h & 2) == 0 ? v : -v);
    };

    int X = (int)floorf(x) & 255;
    int Y = (int)floorf(y) & 255;

    x -= floorf(x);
    y -= floorf(y);

    float u = fade(x);
    float v = fade(y);

    int A = perm[X] + Y;
    int B = perm[X + 1] + Y;

    return Mix(
        Mix(grad(perm[A], x, y), grad(perm[B], x - 1, y), u),
        Mix(grad(perm[A + 1], x, y - 1), grad(perm[B + 1], x - 1, y - 1), u),
        v
    );
}

// layered noise, roughly in [-1, 1]
static float Fbm(float x, float y, int octaves) {
    float sum = 0.0f, amp = 1.0f, norm = 0.0f;
    for (int o = 0; o < octaves; o++) {
        sum += amp * Perlin2D(x, y);
        norm += amp;
        amp *= 0.5f;
        x = x * 2.0f + 17.3f;
        y = y * 2.0f - 41.7f;
    }
    return sum / norm;
}

// repeatable random number in [0, 1) for a world cell
static float Hash01(int x, int z, unsigned int salt) {
    unsigned int h = (unsigned int)x * 374761393u + (unsigned int)z * 668265263u + (terrain_seed + salt) * 2246822519u;
    h = (h ^ (h >> 13)) * 1274126177u;
    h ^= h >> 16;
    return (h & 0xFFFFFF) / 16777216.0f;
}

float TerrainHeight(float wx, float wz) {
    // broad lowlands and highlands, with the hills growing taller on high ground
    float continental = Fbm(wx * 0.005f, wz * 0.005f, 3);
    float hills = Fbm(wx * 0.022f + 37.0f, wz * 0.022f - 91.0f, 4);
    float relief = Mix(6.0f, 24.0f, SmoothStep(-0.15f, 0.3f, continental));
    float h = SEA_LEVEL + 4.5f + continental * 18.0f + hills * relief;

    // rivers run where a second noise crosses zero, pulling their banks down into valleys
    float river = fabsf(Perlin2D(wx * 0.006f - 300.0f, wz * 0.006f + 500.0f));
    float river_bed = SEA_LEVEL - 2.5f;
    if (h > river_bed) h = river_bed + (h - river_bed) * SmoothStep(0.012f, 0.11f, river);
    return h;
}

static float Moisture(float wx, float wz) {
    return Clamp(0.5f + 1.5f * Fbm(wx * 0.012f + 500.0f, wz * 0.012f + 900.0f, 2), 0.0f, 1.0f);
}

// trees are decided purely from world position so neighbouring pieces agree. A site only
// grows a tree if no other site within tree_spacing rolled higher, which keeps canopies from
// overlapping: overlapping boxes have coplanar faces, and those z-fight
static const int tree_spacing = 4;

static bool IsTreeCandidate(int wx, int wz) {
    float roll = Hash01(wx, wz, 7);
    if (roll >= 0.12f) return false;
    return roll < Mix(0.002f, 0.12f, SmoothStep(0.4f, 0.8f, Moisture(wx, wz)));
}

static bool IsTreeSite(int wx, int wz) {
    if (!IsTreeCandidate(wx, wz)) return false;
    float priority = Hash01(wx, wz, 10);
    for (int dz = -tree_spacing; dz <= tree_spacing; dz++) {
        for (int dx = -tree_spacing; dx <= tree_spacing; dx++) {
            if (dx == 0 && dz == 0) continue;
            if (Hash01(wx + dx, wz + dz, 10) > priority && IsTreeCandidate(wx + dx, wz + dz)) return false;
        }
    }
    return true;
}

// track height where it crosses between pieces. Only depends on world position so both sides agree
static float GateHeight(float wx, float wz) {
    float sum = 0.0f;
    for (int dz = -gate_run; dz <= gate_run; dz++) sum += floorf(TerrainHeight(wx, wz + dz));
    return fmaxf(sum / (2 * gate_run + 1), SEA_LEVEL + 1.0f);
}

// ---------------------------------------------------------------- mesh building

static Color Shade(Color c, float s) {
    return (Color){
        (unsigned char)Clamp(c.r * s, 0.0f, 255.0f),
        (unsigned char)Clamp(c.g * s, 0.0f, 255.0f),
        (unsigned char)Clamp(c.b * s, 0.0f, 255.0f),
        c.a
    };
}

static Color MixColor(Color a, Color b, float t) {
    return (Color){
        (unsigned char)Mix(a.r, b.r, t),
        (unsigned char)Mix(a.g, b.g, t),
        (unsigned char)Mix(a.b, b.b, t),
        (unsigned char)Mix(a.a, b.a, t)
    };
}

struct MeshBuilder {
    std::vector<float> vertices;
    std::vector<float> normals;
    std::vector<unsigned char> colors;

    void Vertex(Vector3 p, Vector3 n, Color c) {
        vertices.insert(vertices.end(), {p.x, p.y, p.z});
        normals.insert(normals.end(), {n.x, n.y, n.z});
        colors.insert(colors.end(), {c.r, c.g, c.b, c.a});
    }

    // corners go around the quad in either direction, winding is fixed up to face along n
    void Quad(Vector3 a, Vector3 b, Vector3 c, Vector3 d, Vector3 n, Color ca, Color cb, Color cc, Color cd) {
        Vector3 face = Vector3CrossProduct(Vector3Subtract(b, a), Vector3Subtract(c, a));
        if (Vector3DotProduct(face, n) < 0.0f) {
            std::swap(b, d);
            std::swap(cb, cd);
        }
        // split along the brighter diagonal so baked shading interpolates evenly
        int lum_ac = ca.r + ca.g + ca.b + cc.r + cc.g + cc.b;
        int lum_bd = cb.r + cb.g + cb.b + cd.r + cd.g + cd.b;
        if (lum_ac >= lum_bd) {
            Vertex(a, n, ca); Vertex(b, n, cb); Vertex(c, n, cc);
            Vertex(a, n, ca); Vertex(c, n, cc); Vertex(d, n, cd);
        } else {
            Vertex(b, n, cb); Vertex(c, n, cc); Vertex(d, n, cd);
            Vertex(b, n, cb); Vertex(d, n, cd); Vertex(a, n, ca);
        }
    }

    void Quad(Vector3 a, Vector3 b, Vector3 c, Vector3 d, Vector3 n, Color col) {
        Quad(a, b, c, d, n, col, col, col, col);
    }

    // axis aligned box, sides darken towards the bottom
    void Box(Vector3 lo, Vector3 hi, Color top, Color side, float bottom_shade, bool draw_bottom) {
        Color low = Shade(side, bottom_shade);
        Quad({lo.x, hi.y, lo.z}, {lo.x, hi.y, hi.z}, {hi.x, hi.y, hi.z}, {hi.x, hi.y, lo.z}, {0, 1, 0}, top);
        Quad({hi.x, lo.y, lo.z}, {hi.x, lo.y, hi.z}, {hi.x, hi.y, hi.z}, {hi.x, hi.y, lo.z}, {1, 0, 0}, low, low, side, side);
        Quad({lo.x, lo.y, lo.z}, {lo.x, lo.y, hi.z}, {lo.x, hi.y, hi.z}, {lo.x, hi.y, lo.z}, {-1, 0, 0}, low, low, side, side);
        Quad({lo.x, lo.y, hi.z}, {hi.x, lo.y, hi.z}, {hi.x, hi.y, hi.z}, {lo.x, hi.y, hi.z}, {0, 0, 1}, low, low, side, side);
        Quad({lo.x, lo.y, lo.z}, {hi.x, lo.y, lo.z}, {hi.x, hi.y, lo.z}, {lo.x, hi.y, lo.z}, {0, 0, -1}, low, low, side, side);
        if (draw_bottom) {
            Quad({lo.x, lo.y, lo.z}, {lo.x, lo.y, hi.z}, {hi.x, lo.y, hi.z}, {hi.x, lo.y, lo.z}, {0, -1, 0}, Shade(low, 0.8f));
        }
    }

    // bar lying on the ground between a and b, used for rails and sleepers
    void Beam(Vector3 a, Vector3 b, float half_wid, float height, Color col) {
        Vector3 along = Vector3Subtract(b, a);
        Vector3 side = Vector3Scale(Vector3Normalize((Vector3){-along.z, 0.0f, along.x}), half_wid);
        Vector3 up = {0.0f, height, 0.0f};
        Vector3 al = Vector3Subtract(a, side), ar = Vector3Add(a, side);
        Vector3 bl = Vector3Subtract(b, side), br = Vector3Add(b, side);
        Color dark = Shade(col, 0.7f);
        Quad(Vector3Add(al, up), Vector3Add(ar, up), Vector3Add(br, up), Vector3Add(bl, up), {0, 1, 0}, col);
        Quad(ar, br, Vector3Add(br, up), Vector3Add(ar, up), Vector3Normalize(side), dark);
        Quad(al, bl, Vector3Add(bl, up), Vector3Add(al, up), Vector3Negate(Vector3Normalize(side)), dark);
    }

    // continuous bar along a line of points. Neighbouring sections share their ends, where
    // separate bars would overlap on curves and z-fight
    void Ribbon(const std::vector<Vector3>& line, const std::vector<Vector3>& across, float half_wid, float height, Color col) {
        Color dark = Shade(col, 0.7f);
        Vector3 up = {0.0f, height, 0.0f};
        for (size_t i = 0; i + 1 < line.size(); i++) {
            Vector3 al = Vector3Subtract(line[i], Vector3Scale(across[i], half_wid));
            Vector3 ar = Vector3Add(line[i], Vector3Scale(across[i], half_wid));
            Vector3 bl = Vector3Subtract(line[i + 1], Vector3Scale(across[i + 1], half_wid));
            Vector3 br = Vector3Add(line[i + 1], Vector3Scale(across[i + 1], half_wid));
            Vector3 side = Vector3Normalize(Vector3Add(across[i], across[i + 1]));
            Quad(Vector3Add(al, up), Vector3Add(ar, up), Vector3Add(br, up), Vector3Add(bl, up), {0, 1, 0}, col);
            Quad(ar, br, Vector3Add(br, up), Vector3Add(ar, up), side, dark);
            Quad(al, bl, Vector3Add(bl, up), Vector3Add(al, up), Vector3Negate(side), dark);
        }
    }

    Model Build() {
        Mesh mesh = {0};
        mesh.vertexCount = (int)(vertices.size() / 3);
        mesh.triangleCount = mesh.vertexCount / 3;
        mesh.vertices = (float *)MemAlloc(vertices.size() * sizeof(float));
        mesh.normals = (float *)MemAlloc(normals.size() * sizeof(float));
        mesh.colors = (unsigned char *)MemAlloc(colors.size());
        // untextured, but the GLES2 path binds a texcoord buffer regardless
        mesh.texcoords = (float *)MemAlloc(mesh.vertexCount * 2 * sizeof(float));
        memcpy(mesh.vertices, vertices.data(), vertices.size() * sizeof(float));
        memcpy(mesh.normals, normals.data(), normals.size() * sizeof(float));
        memcpy(mesh.colors, colors.data(), colors.size());
        UploadMesh(&mesh, false);
        return LoadModelFromMesh(mesh);
    }
};

static Color BlockTopColor(Block block, float h, float moisture, float jitter) {
    Color c;
    switch (block) {
        case GRAVEL:  c = (Color){112, 106, 100, 255}; break;
        case BALLAST: c = (Color){138, 131, 120, 255}; break;
        case SAND:    c = (Color){218, 203, 144, 255}; break;
        case SEABED:  c = Shade((Color){170, 160, 118, 255}, Clamp(1.0f + (h - SEA_LEVEL) * 0.12f, 0.45f, 1.0f)); break;
        case STONE:   c = (Color){128, 128, 130, 255}; break;
        case SNOW:    c = (Color){238, 242, 247, 255}; break;
        default:
            c = MixColor((Color){158, 168, 82, 255}, (Color){84, 152, 58, 255}, moisture);
            c = Shade(c, 1.0f - Clamp((h - 6.0f) * 0.012f, 0.0f, 0.15f));
            break;
    }
    return Shade(c, 0.94f + 0.12f * jitter);
}

static Color BlockSideColor(Block block) {
    switch (block) {
        case GRAVEL:
        case BALLAST: return (Color){104, 98, 92, 255};
        case SAND:
        case SEABED:  return (Color){190, 174, 122, 255};
        case GRASS:   return (Color){124, 88, 58, 255};
        default:      return (Color){112, 112, 116, 255};
    }
}

static void AddOak(MeshBuilder& mb, float x, float y, float z, float r1, float r2) {
    float trunk = 3.0f + floorf(r1 * 3.0f);
    Color bark = Shade((Color){104, 74, 46, 255}, 0.9f + 0.2f * r2);
    Color leaf = Shade((Color){58, 128, 48, 255}, 0.85f + 0.3f * r2);
    mb.Box({x - 0.35f, y, z - 0.35f}, {x + 0.35f, y + trunk, z + 0.35f}, bark, bark, 0.75f, false);
    // leaf faces sit off the block grid so they never line up with terrain faces
    mb.Box({x - 2.4f, y + trunk - 0.9f, z - 2.4f}, {x + 2.4f, y + trunk + 1.1f, z + 2.4f}, leaf, Shade(leaf, 0.85f), 0.7f, true);
    mb.Box({x - 1.4f, y + trunk + 1.1f, z - 1.4f}, {x + 1.4f, y + trunk + 2.4f, z + 1.4f}, Shade(leaf, 1.08f), Shade(leaf, 0.92f), 0.85f, false);
}

static void AddPine(MeshBuilder& mb, float x, float y, float z, float r1, float r2) {
    float trunk = 2.0f + floorf(r1 * 2.0f);
    Color bark = Shade((Color){86, 62, 44, 255}, 0.9f + 0.2f * r2);
    Color leaf = Shade((Color){38, 92, 62, 255}, 0.85f + 0.3f * r2);
    mb.Box({x - 0.3f, y, z - 0.3f}, {x + 0.3f, y + trunk, z + 0.3f}, bark, bark, 0.75f, false);
    mb.Box({x - 1.9f, y + trunk + 0.1f, z - 1.9f}, {x + 1.9f, y + trunk + 1.2f, z + 1.9f}, leaf, Shade(leaf, 0.85f), 0.7f, true);
    mb.Box({x - 1.3f, y + trunk + 1.2f, z - 1.3f}, {x + 1.3f, y + trunk + 3.0f, z + 1.3f}, Shade(leaf, 1.06f), Shade(leaf, 0.9f), 0.75f, false);
    mb.Box({x - 0.6f, y + trunk + 3.0f, z - 0.6f}, {x + 0.6f, y + trunk + 5.0f, z + 0.6f}, Shade(leaf, 1.12f), Shade(leaf, 0.95f), 0.8f, false);
}

// ---------------------------------------------------------------- stage pieces

static void BuildPath(StagePiece& piece, bool straight_path) {
    Grid& grid = piece.grid;
    int grid_width = (int)sqrt(grid.cells.size());
    int half = grid_width / 2;
    int last = grid_width - 1;

    for (size_t i = 0; i < grid.cells.size(); i++) {
        Vector3 world = Vector3Add(grid.draw_positions[i], piece.position);
        grid.cells[i].height = floorf(TerrainHeight(world.x, world.z));
    }

    // the track always enters and leaves a piece dead straight through the middle of its
    // edge, only the stretch in between is free to wander. Path x runs along world z, so
    // the train travels in -x (direction 1)
    std::vector<Vector2> wander;
    if (!straight_path) {
        int start[2] = {last - gate_run, half};
        int end[2] = {gate_run, half};
        wander = PathFinder::FindPath(grid, start, end, 1, min_run, 6.0f);
    }
    if (wander.empty()) {
        for (int x = last - gate_run; x >= gate_run; x--) wander.push_back((Vector2){(float)x, (float)half});
    }

    piece.path.clear();
    for (int x = last; x > last - gate_run; x--) piece.path.push_back((Vector2){(float)x, (float)half});
    piece.path.insert(piece.path.end(), wander.begin(), wander.end());
    for (int x = gate_run - 1; x >= 0; x--) piece.path.push_back((Vector2){(float)x, (float)half});

    // track height for each block: follows the ground but stays clear of the water, relaxed
    // into gentle grades
    int cells = (int)piece.path.size();
    float gate_start = GateHeight(piece.position.x, piece.position.z + half);
    float gate_end = GateHeight(piece.position.x, piece.position.z - half);
    std::vector<Vector3> blocks(cells);
    std::vector<float> profile(cells);
    for (int i = 0; i < cells; i++) {
        int index = (int)(piece.path[i].y * grid_width + piece.path[i].x);
        blocks[i] = grid.draw_positions[index];
        profile[i] = fmaxf(grid.cells[index].height, SEA_LEVEL + 1.0f);
        if (i <= gate_run) profile[i] = gate_start;
        if (i >= cells - 1 - gate_run) profile[i] = gate_end;
    }
    std::vector<float> relaxed = profile;
    for (int pass = 0; pass < 40; pass++) {
        for (int i = gate_run + 1; i < cells - 1 - gate_run; i++) {
            relaxed[i] = (profile[i - 1] + profile[i] + profile[i + 1]) / 3.0f;
        }
        profile = relaxed;
    }
    // height partway along the blocks, for points that fall between block centres
    auto height_at = [&](float along) {
        int i = std::min(std::max((int)floorf(along), 0), cells - 2);
        return Mix(profile[i], profile[i + 1], Clamp(along - i, 0.0f, 1.0f));
    };

    // the rails run through the middle of each block. Where the path turns they bend round a
    // quarter circle that starts and ends corner_radius either side of the corner block
    auto is_corner = [&](int i) {
        if (i <= 0 || i >= cells - 1) return false;
        Vector3 in = Vector3Subtract(blocks[i], blocks[i - 1]);
        Vector3 out = Vector3Subtract(blocks[i + 1], blocks[i]);
        return fabsf(in.x - out.x) + fabsf(in.z - out.z) > 0.5f;
    };
    const int arc_steps = 8;
    piece.track.clear();
    piece.track_sleeper.clear();
    for (int i = 0; i < cells; i++) {
        if (is_corner(i)) {
            Vector3 in = Vector3Subtract(blocks[i], blocks[i - 1]);
            Vector3 out = Vector3Subtract(blocks[i + 1], blocks[i]);
            Vector3 arc_start = Vector3Subtract(blocks[i], Vector3Scale(in, corner_radius));
            Vector3 centre = Vector3Add(arc_start, Vector3Scale(out, corner_radius));
            for (int k = 0; k <= arc_steps; k++) {
                float angle = (PI / 2.0f) * k / arc_steps;
                Vector3 p = Vector3Add(centre, Vector3Add(Vector3Scale(out, -corner_radius * cosf(angle)),
                                                          Vector3Scale(in, corner_radius * sinf(angle))));
                p.y = height_at(i - corner_radius + 2.0f * corner_radius * k / arc_steps);
                piece.track.push_back(p);
                piece.track_sleeper.push_back(k == 2 || k == 4 || k == 6);
            }
        } else if (!is_corner(i - 1) && !is_corner(i + 1)) {
            Vector3 p = blocks[i];
            p.y = profile[i];
            piece.track.push_back(p);
            piece.track_sleeper.push_back(true);
        }
    }

    int n = (int)piece.track.size();
    piece.track_s.resize(n);
    piece.track_s[0] = 0.0f;
    for (int i = 1; i < n; i++) piece.track_s[i] = piece.track_s[i - 1] + Vector3Distance(piece.track[i - 1], piece.track[i]);
    piece.track_length = piece.track_s[n - 1];
}

void BuildStagePiece(StagePiece& piece, bool straight_path) {
    BuildPath(piece, straight_path);

    Grid& grid = piece.grid;
    int grid_width = (int)sqrt(grid.cells.size());
    int half = grid_width / 2;
    int n = (int)piece.track.size();

    // track in local space, continued a little way into the neighbouring pieces. Their
    // track is straight and level there, so we know it without generating them
    std::vector<Vector3> track;
    for (int k = gate_run; k >= 1; k--) track.push_back((Vector3){0.0f, piece.track[0].y, (float)(half + k)});
    track.insert(track.end(), piece.track.begin(), piece.track.end());
    for (int k = 1; k <= gate_run; k++) track.push_back((Vector3){0.0f, piece.track[n - 1].y, (float)(-half - k)});

    // column heights for the piece plus a one cell border, so faces and shading at the
    // edges match the neighbours. A piece owns local z in [-half, half)
    const int nx = 2 * half_width + 2;
    const int nz = 2 * half + 2;
    std::vector<float> heights(nx * nz);
    std::vector<float> track_dist(nx * nz);
    std::vector<float> track_side(nx * nz); // x offset from the nearest bit of track
    auto at = [&](int lx, int lz) { return (lx + half_width + 1) + (lz + half + 1) * nx; };

    for (int lz = -half - 1; lz <= half; lz++) {
        for (int lx = -half_width - 1; lx <= half_width; lx++) {
            float natural = floorf(TerrainHeight(piece.position.x + lx, piece.position.z + lz));
            float nearest = 1e9f;
            float track_y = 0.0f;
            float side = 0.0f;
            for (const Vector3& p : track) {
                float d = (p.x - lx) * (p.x - lx) + (p.z - lz) * (p.z - lz);
                if (d < nearest) {
                    nearest = d;
                    track_y = p.y;
                    side = lx - p.x;
                }
            }
            nearest = sqrtf(nearest);

            // water is left alone, the track crosses it on a bridge
            float h = natural;
            if (natural < SEA_LEVEL) {
                h = natural;
            } else if (nearest <= bed_radius) {
                h = track_y;
            } else if (nearest < carve_radius) {
                h = roundf(Mix(track_y, natural, SmoothStep(bed_radius, carve_radius, nearest)));
            }
            heights[at(lx, lz)] = h;
            track_dist[at(lx, lz)] = nearest;
            track_side[at(lx, lz)] = side;
        }
    }

    MeshBuilder mb;
    piece.has_water = false;
    const int dirs[4][2] = {{1, 0}, {-1, 0}, {0, 1}, {0, -1}};
    const float ao_levels[4] = {1.0f, 0.8f, 0.66f, 0.52f};

    for (int lz = -half; lz < half; lz++) {
        for (int lx = -half_width; lx < half_width; lx++) {
            int wx = (int)piece.position.x + lx;
            int wz = (int)piece.position.z + lz;
            float h = heights[at(lx, lz)];
            float dist = track_dist[at(lx, lz)];
            float moisture = Moisture(wx, wz);

            float slope = 0.0f;
            for (auto& d : dirs) slope = fmaxf(slope, fabsf(heights[at(lx + d[0], lz + d[1])] - h));

            Block block = GRASS;
            if (dist <= 1.6f) block = GRAVEL;
            else if (dist <= bed_radius + 0.6f) block = BALLAST;
            else if (h < SEA_LEVEL) block = SEABED;
            else if (h < SEA_LEVEL + 1) block = SAND;
            else if (h >= 17.0f + 3.0f * Hash01(wx, wz, 3)) block = SNOW;
            else if (slope >= 3.0f || h >= 13.0f + 2.0f * Hash01(wx, wz, 4)) block = STONE;

            if (h < SEA_LEVEL) piece.has_water = true;

            // top face, corners darkened by taller neighbours
            Color top = BlockTopColor(block, h, moisture, Hash01(wx, wz, 1));
            Vector3 corners[4];
            Color corner_colors[4];
            const int signs[4][2] = {{-1, -1}, {-1, 1}, {1, 1}, {1, -1}};
            for (int c = 0; c < 4; c++) {
                int sx = signs[c][0], sz = signs[c][1];
                bool side_a = heights[at(lx + sx, lz)] > h + 0.5f;
                bool side_b = heights[at(lx, lz + sz)] > h + 0.5f;
                bool diagonal = heights[at(lx + sx, lz + sz)] > h + 0.5f;
                int occlusion = (side_a && side_b) ? 3 : side_a + side_b + diagonal;
                corners[c] = (Vector3){lx + 0.5f * sx, h, lz + 0.5f * sz};
                corner_colors[c] = Shade(top, ao_levels[occlusion]);
            }
            mb.Quad(corners[0], corners[1], corners[2], corners[3], {0, 1, 0},
                    corner_colors[0], corner_colors[1], corner_colors[2], corner_colors[3]);

            // side faces down to any lower neighbour: a band of soil, then rock
            Color soil = BlockSideColor(block);
            Color rock = BlockSideColor(STONE);
            for (auto& d : dirs) {
                float neighbour = heights[at(lx + d[0], lz + d[1])];
                if (neighbour >= h) continue;
                Vector3 normal = {(float)d[0], 0.0f, (float)d[1]};
                float cx = lx + 0.5f * d[0], cz = lz + 0.5f * d[1];
                float tx = -0.5f * d[1], tz = 0.5f * d[0];
                auto shade_at = [&](float y) { return 0.68f + 0.32f * Clamp((y - neighbour) / 1.5f, 0.0f, 1.0f); };
                auto band = [&](float lo, float hi, Color col) {
                    Color cl = Shade(col, shade_at(lo)), ch = Shade(col, shade_at(hi));
                    mb.Quad({cx - tx, lo, cz - tz}, {cx + tx, lo, cz + tz}, {cx + tx, hi, cz + tz}, {cx - tx, hi, cz - tz},
                            normal, cl, cl, ch, ch);
                };
                float soil_bottom = fmaxf(neighbour, h - 2.0f);
                band(soil_bottom, h, soil);
                if (neighbour < soil_bottom) band(neighbour, soil_bottom, rock);
            }

            // trees grow in clumps where it's damp, kept back from the track
            float clearance = track_side[at(lx, lz)] > 0.0f ? tree_clearance_near : tree_clearance;
            if (block == GRASS && slope <= 1.0f && dist > clearance && IsTreeSite(wx, wz)) {
                float r1 = Hash01(wx, wz, 8), r2 = Hash01(wx, wz, 9);
                if (h >= 8.0f + 3.0f * r2) AddPine(mb, lx, h, lz, r1, r2);
                else AddOak(mb, lx, h, lz, r1, r2);
            }
        }
    }

    // sleepers and rails along this piece's own stretch of track. Everything sits a little into
    // the ground (track y is the top of the bed) so nothing is flush with a block face
    Color sleeper = {92, 66, 44, 255};
    Color steel = {168, 172, 180, 255};
    const float sleeper_base = -0.06f;
    const float sleeper_top = 0.12f;
    std::vector<Vector3> centre_line, across_line, left_rail, right_rail;
    for (int i = 0; i < n; i++) {
        const Vector3& p = track[gate_run + i];
        const Vector3& before = track[gate_run + i - 1];
        const Vector3& after = track[gate_run + i + 1];
        Vector3 across = Vector3Normalize((Vector3){-(after.z - before.z), 0.0f, after.x - before.x});
        centre_line.push_back(p);
        across_line.push_back(across);

        Vector3 base = {p.x, p.y + sleeper_base, p.z};
        if (i < n - 1 && piece.track_sleeper[i]) mb.Beam(Vector3Add(base, Vector3Scale(across, -1.1f)), Vector3Add(base, Vector3Scale(across, 1.1f)), 0.18f, sleeper_top - sleeper_base, sleeper);
        Vector3 rail_base = {p.x, p.y + sleeper_top, p.z};
        left_rail.push_back(Vector3Add(rail_base, Vector3Scale(across, -0.6f)));
        right_rail.push_back(Vector3Add(rail_base, Vector3Scale(across, 0.6f)));
    }
    mb.Ribbon(left_rail, across_line, 0.07f, rail_height - sleeper_top, steel);
    mb.Ribbon(right_rail, across_line, 0.07f, rail_height - sleeper_top, steel);

    // bridges wherever the track is over water: a deck under the sleepers, its top just
    // below the bed so it disappears into the banks at each end, and piers every few blocks
    Color planks = {134, 100, 66, 255};
    Color pier = {120, 120, 124, 255};
    const float deck_thickness = 0.35f;
    auto over_water = [&](const Vector3& p) {
        return TerrainHeight(piece.position.x + roundf(p.x), piece.position.z + roundf(p.z)) < SEA_LEVEL;
    };
    std::vector<Vector3> deck, deck_across;
    for (int i = 0; i < n; i++) {
        bool bridged = over_water(centre_line[i]) ||
                       (i > 0 && over_water(centre_line[i - 1])) ||
                       (i + 1 < n && over_water(centre_line[i + 1]));
        if (bridged) {
            const Vector3& p = centre_line[i];
            deck.push_back((Vector3){p.x, p.y + sleeper_base - deck_thickness, p.z});
            deck_across.push_back(across_line[i]);
            int wz = (int)(piece.position.z + roundf(p.z));
            if (over_water(p) && wz % 4 == 0) {
                float bed = floorf(TerrainHeight(piece.position.x + roundf(p.x), piece.position.z + roundf(p.z)));
                mb.Box({p.x - 1.3f, bed, p.z - 0.4f}, {p.x + 1.3f, deck.back().y, p.z + 0.4f}, pier, pier, 0.6f, false);
            }
        }
        if ((!bridged || i == n - 1) && deck.size() > 1) {
            mb.Ribbon(deck, deck_across, 1.7f, deck_thickness, planks);
        }
        if (!bridged) {
            deck.clear();
            deck_across.clear();
        }
    }

    piece.terrain = mb.Build();
    piece.terrain.materials[0].shader = terrain_shader;
    piece.terrain.materials[0].maps[MATERIAL_MAP_METALNESS].texture = shadow_target.texture;

    if (piece.has_water) {
        MeshBuilder wb;
        float y = SEA_LEVEL - 0.2f;
        float x0 = -half_width - 0.5f, x1 = half_width - 0.5f;
        float z0 = -half - 0.5f, z1 = half - 0.5f;
        wb.Quad({x0, y, z0}, {x0, y, z1}, {x1, y, z1}, {x1, y, z0}, {0, 1, 0}, (Color){52, 118, 196, 165});
        piece.water = wb.Build();
        piece.water.materials[0].shader = water_shader;
    }
    piece.models_loaded = true;
}

void UnloadStagePiece(StagePiece& piece) {
    if (!piece.models_loaded) return;
    UnloadModel(piece.terrain);
    if (piece.has_water) UnloadModel(piece.water);
    piece.models_loaded = false;
    piece.has_water = false;
}

Vector3 PathPoint(const StagePiece& piece, size_t i) {
    Vector3 pos = Vector3Add(piece.track[i], piece.position);
    pos.y += rail_height;
    return pos;
}

// ---------------------------------------------------------------- shaders

void InitTerrain(int seed) {
    terrain_seed = (unsigned int)seed;
    SetRandomSeed(terrain_seed);
    for (int i = 0; i < 256; i++) perm[i] = i;
    for (int i = 255; i > 0; i--) {
        int j = GetRandomValue(0, i);
        unsigned char temp = perm[i];
        perm[i] = perm[j];
        perm[j] = temp;
    }
    for (int i = 0; i < 256; i++) perm[256 + i] = perm[i];

    terrain_shader = LoadShader(TextFormat("../assets/shaders/glsl%i/terrain.vs", GLSL_VERSION),
                                TextFormat("../assets/shaders/glsl%i/terrain.fs", GLSL_VERSION));
    water_shader = LoadShader(TextFormat("../assets/shaders/glsl%i/terrain.vs", GLSL_VERSION),
                              TextFormat("../assets/shaders/glsl%i/water.fs", GLSL_VERSION));

    sun_dir = Vector3Normalize((Vector3){-0.45f, -1.0f, -0.3f});
    float sun_color[3] = {1.0f, 0.95f, 0.84f};
    float ambient[3] = {0.5f, 0.56f, 0.66f};
    float fog_color[3] = {sky_color.r / 255.0f, sky_color.g / 255.0f, sky_color.b / 255.0f};
    float fog_start = 75.0f, fog_end = 165.0f;
    Shader shaders[2] = {terrain_shader, water_shader};
    for (Shader& shader : shaders) {
        SetShaderValue(shader, GetShaderLocation(shader, "sunDir"), &sun_dir, SHADER_UNIFORM_VEC3);
        SetShaderValue(shader, GetShaderLocation(shader, "sunColor"), sun_color, SHADER_UNIFORM_VEC3);
        SetShaderValue(shader, GetShaderLocation(shader, "ambientColor"), ambient, SHADER_UNIFORM_VEC3);
        SetShaderValue(shader, GetShaderLocation(shader, "fogColor"), fog_color, SHADER_UNIFORM_VEC3);
        SetShaderValue(shader, GetShaderLocation(shader, "fogStart"), &fog_start, SHADER_UNIFORM_FLOAT);
        SetShaderValue(shader, GetShaderLocation(shader, "fogEnd"), &fog_end, SHADER_UNIFORM_FLOAT);
    }
    // shadow map: the scene's depth from the sun, drawn each frame before the real pass.
    // It rides in a spare material map slot so DrawModel binds it for us
    depth_shader = LoadShader(TextFormat("../assets/shaders/glsl%i/shadow_depth.vs", GLSL_VERSION),
                              TextFormat("../assets/shaders/glsl%i/shadow_depth.fs", GLSL_VERSION));
    shadow_target = LoadRenderTexture(shadow_resolution, shadow_resolution);
    terrain_shader.locs[SHADER_LOC_MAP_METALNESS] = GetShaderLocation(terrain_shader, "shadowMap");
    float shadow_texel = 1.5f / shadow_resolution;
    SetShaderValue(terrain_shader, GetShaderLocation(terrain_shader, "shadowTexel"), &shadow_texel, SHADER_UNIFORM_FLOAT);
    terrain_light_loc = GetShaderLocation(terrain_shader, "lightVP");
    depth_light_loc = GetShaderLocation(depth_shader, "lightVP");

    terrain_view_loc = GetShaderLocation(terrain_shader, "viewPos");
    water_view_loc = GetShaderLocation(water_shader, "viewPos");
    water_time_loc = GetShaderLocation(water_shader, "time");
}

void UnloadTerrain() {
    UnloadShader(terrain_shader);
    UnloadShader(water_shader);
    UnloadShader(depth_shader);
    UnloadRenderTexture(shadow_target);
}

Shader GetTerrainShader() { return shadow_pass ? depth_shader : terrain_shader; }

Texture2D GetShadowMap() { return shadow_target.texture; }

void BeginShadowPass(Vector3 focus) {
    // move the shadowed area in whole texels, otherwise shadow edges crawl as the train moves
    Vector3 right = Vector3Normalize(Vector3CrossProduct(sun_dir, (Vector3){0.0f, 1.0f, 0.0f}));
    Vector3 up = Vector3CrossProduct(right, sun_dir);
    float texel = 2.0f * shadow_extent / shadow_resolution;
    float along_right = Vector3DotProduct(focus, right);
    float along_up = Vector3DotProduct(focus, up);
    focus = Vector3Add(focus, Vector3Scale(right, roundf(along_right / texel) * texel - along_right));
    focus = Vector3Add(focus, Vector3Scale(up, roundf(along_up / texel) * texel - along_up));

    Camera3D sun = {0};
    sun.position = Vector3Subtract(focus, Vector3Scale(sun_dir, 150.0f));
    sun.target = focus;
    sun.up = (Vector3){0.0f, 1.0f, 0.0f};
    sun.fovy = 2.0f * shadow_extent;
    sun.projection = CAMERA_ORTHOGRAPHIC;

    Matrix light_vp = MatrixMultiply(MatrixLookAt(sun.position, sun.target, sun.up),
                                     MatrixOrtho(-shadow_extent, shadow_extent, -shadow_extent, shadow_extent, 1.0, 300.0));
    SetShaderValueMatrix(terrain_shader, terrain_light_loc, light_vp);
    SetShaderValueMatrix(depth_shader, depth_light_loc, light_vp);

    BeginTextureMode(shadow_target);
    ClearBackground(WHITE);
    BeginMode3D(sun);
    shadow_pass = true;
}

void EndShadowPass() {
    shadow_pass = false;
    EndMode3D();
    EndTextureMode();
}

Color GetSkyColor() { return sky_color; }

void UpdateTerrainShaders(Camera3D camera, float time) {
    SetShaderValue(terrain_shader, terrain_view_loc, &camera.position, SHADER_UNIFORM_VEC3);
    SetShaderValue(water_shader, water_view_loc, &camera.position, SHADER_UNIFORM_VEC3);
    SetShaderValue(water_shader, water_time_loc, &time, SHADER_UNIFORM_FLOAT);
}
