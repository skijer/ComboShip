#include "vertex_fixture.inc"

static uint32_t seed = 17;
static uint32_t randomBits() {
    seed ^= seed << 13;
    seed ^= seed >> 17;
    seed ^= seed << 5;
    return seed;
}
static void setupVertices(Fixture& f, F3DVtx* vertices, unsigned mode, unsigned repeats) {
    auto& r = *f.gfx.mRsp;
    r.geometry_mode = mode;
    r.modelview_matrix_stack_size = 1;
    for (int i = 0; i < 4; ++i)
        for (int j = 0; j < 4; ++j) {
            r.MP_matrix[i][j] = int(randomBits() % 1025 - 512) / 128.f;
            r.modelview_matrix_stack[0][i][j] = int(randomBits() % 1025 - 512) / 512.f;
        }
    if (seed % 4 == 0) {
        for (int i = 0; i < 3; ++i)
            r.MP_matrix[i][3] = 0.0f;
        r.MP_matrix[3][3] = seed % 3 == 0 ? -0.0001f : seed % 3 == 1 ? 0.0f : 0.0001f;
    }
    r.current_num_lights = 1 + randomBits() % (MAX_LIGHTS + 1);
    for (unsigned i = 0; i < MAX_LIGHTS + 1; ++i) {
        auto& l = r.current_lights[i];
        for (int j = 0; j < 3; ++j) {
            l.l.col[j] = randomBits();
            l.l.dir[j] = randomBits();
        }
        if (mode & G_LIGHTING_POSITIONAL) {
            l.p.unk3 = i % 2;
            l.p.unk7 = randomBits();
            l.p.unkE = randomBits();
            for (int j = 0; j < 3; ++j)
                l.p.pos[j] = int(randomBits() % 1000) - 500;
        }
    }
    for (auto& l : r.lookat)
        for (int j = 0; j < 3; ++j)
            l.dir[j] = randomBits();
    r.lights_changed = true;
    r.fog_mul = randomBits();
    r.fog_offset = randomBits();
    r.texture_scaling_factor.s = randomBits();
    r.texture_scaling_factor.t = randomBits();
    f.gfx.mCurDimensions.width = 640 + randomBits() % 1000;
    f.gfx.mCurDimensions.height = 480;
    for (unsigned i = 0; i < 32; ++i) {
        auto& v = vertices[i];
        for (int j = 0; j < 3; ++j) {
            v.v.ob[j] = randomBits();
            v.n.n[j] = randomBits();
        }
        if (i && i % 100 < repeats)
            for (int j = 0; j < 3; ++j)
                v.n.n[j] = vertices[i - 1].n.n[j];
        v.v.tc[0] = randomBits();
        v.v.tc[1] = randomBits();
        v.v.cn[3] = randomBits();
    }
}
static void compare(const Fixture& a, const Fixture& b) {
    for (unsigned i = 0; i < MAX_VERTICES; ++i) {
        const auto& x = a.gfx.mRsp->loaded_vertices[i];
        const auto& y = b.gfx.mRsp->loaded_vertices[i];
        require(memcmp(&x.x, &y.x, 6 * sizeof(float)) == 0, "vertex position/UV bitwise equality");
        require(memcmp(&x.color, &y.color, sizeof(x.color)) == 0, "vertex color/fog exact equality");
        require(x.clip_rej == y.clip_rej, "vertex clip equality");
    }
    require(memcmp(a.gfx.mRsp->current_lookat_coeffs, b.gfx.mRsp->current_lookat_coeffs,
                   sizeof(a.gfx.mRsp->current_lookat_coeffs)) == 0,
            "lookat coefficients equality");
    require(a.gfx.mRsp->lights_changed == b.gfx.mRsp->lights_changed, "light dirty state equality");
    require(memcmp(a.gfx.mRsp->current_lights_coeffs, b.gfx.mRsp->current_lights_coeffs,
                   sizeof(a.gfx.mRsp->current_lights_coeffs)) == 0,
            "light coefficients equality");
}
int main(int argc, char** argv) {
    bool bench = false, skipNull = false;
    for (int i = 1; i < argc; ++i) {
        bench |= std::string(argv[i]) == "--benchmark";
        skipNull |= std::string(argv[i]) == "--skip-null-test";
    }
    for (unsigned k = 0; k < 2048; ++k) {
        Fixture a, b;
        F3DVtx v[32]{};
        unsigned mode = ((k & 1) ? G_LIGHTING : 0) | ((k & 2) ? G_LIGHTING_POSITIONAL : 0) |
                        ((k & 4) ? G_TEXTURE_GEN : 0) | ((k & 8) ? G_TEXTURE_GEN_LINEAR : 0) | ((k & 16) ? G_FOG : 0);
        setupVertices(a, v, mode, k % 33);
        *b.gfx.mRsp = *a.gfx.mRsp;
        b.gfx.mCurDimensions = a.gfx.mCurDimensions;
        for (auto* f : { &a, &b }) {
            f->gfx.mFbActive = k % 4 != 0;
            f->gfx.mFrameBuffers[1] = {};
            f->gfx.mFrameBuffers[1].resize = (k % 4 == 1);
            f->gfx.mFrameBuffers[1].forceFixedAspect = (k % 4 == 2);
            f->gfx.mActiveFrameBuffer = k % 4 == 3 ? f->gfx.mFrameBuffers.end() : f->gfx.mFrameBuffers.begin();
        }
        if (!skipNull) {
            a.gfx.GfxSpVertex(1, 0, nullptr);
            b.gfx.GfxSpVertexBaseline(1, 0, nullptr);
            compare(a, b);
        }
        for (int pass = 0; pass < 4; ++pass) {
            if (pass == 3) {
                // A repeated first normal must not reuse a preceding command's result.
                a.gfx.mRsp->current_lights[0].l.col[0] ^= 255;
                a.gfx.mRsp->lookat[0].dir[0] ^= 127;
                a.gfx.mRsp->modelview_matrix_stack[0][0][0] += .125f;
                a.gfx.mRsp->lights_changed = true;
                *b.gfx.mRsp = *a.gfx.mRsp;
            }
            unsigned n = pass == 0 ? 0 : pass == 1 ? 32 : k % 32;
            a.gfx.GfxSpVertex(n, 3, v);
            b.gfx.GfxSpVertexBaseline(n, 3, v);
            compare(a, b);
        }
    }
    std::cout << "PASS " << (skipNull ? 8192 : 10240) << " production/baseline vertex batches"
              << (skipNull ? " (existing null UB case excluded)" : " (including null and changed state)") << '\n';
    if (!bench)
        return 0;
    for (unsigned mode :
         { 0u, unsigned(G_LIGHTING), unsigned(G_LIGHTING | G_TEXTURE_GEN),
           unsigned(G_LIGHTING | G_TEXTURE_GEN | G_TEXTURE_GEN_LINEAR), unsigned(G_LIGHTING | G_LIGHTING_POSITIONAL) })
        for (unsigned reuse : { 0u, 16u, 32u }) {
            Fixture f;
            F3DVtx v[32]{};
            seed = 12345;
            setupVertices(f, v, mode, reuse);
            f.gfx.mRsp->current_num_lights = 4;
            std::vector<double> times[2];
            for (int rep = 0; rep < 12; ++rep)
                for (int step = 0; step < 2; ++step) {
                    int baseline = (rep + step) % 2;
                    auto start = std::chrono::steady_clock::now();
                    for (int batch = 0; batch < 6250; ++batch) {
                        if (baseline)
                            f.gfx.GfxSpVertexBaseline(32, 0, v);
                        else
                            f.gfx.GfxSpVertex(32, 0, v);
                    }
                    auto end = std::chrono::steady_clock::now();
                    if (rep)
                        times[baseline].push_back(std::chrono::duration<double, std::milli>(end - start).count());
                }
            for (auto& t : times)
                std::sort(t.begin(), t.end());
            std::cout << "BENCH mode=" << mode << " reuse=" << reuse << " baseline_ms=" << times[1][5]
                      << " candidate_ms=" << times[0][5] << '\n';
        }
}
