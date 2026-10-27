/* Optional diagnostic export of the actual selector's submitted GBI commands. */
static void elemental_ui_dump(const char* prefix) {
    struct Texture { const u8* pixels; int width, height, format, native; } textures[128];
    const u8* pending = NULL;
    int format = 0, texture = -1, count = 0, gray = 0;
    u32 color = 0xffffffff;
    char path[1024];
    snprintf(path, sizeof(path), "%s.json", prefix);
    FILE* output = fopen(path, "w");
    assert(output);
    fprintf(output, "{\"rectangles\":[");
    int first = 1;
    for (Gfx* g = sOverlay; g < sGfx.overlay.p; ++g) {
        int op = g->words.w0 >> 24;
        if (op == G_SETPRIMCOLOR) color = g->words.w1;
        if (op == G_SETGRAYSCALE) gray = g->words.w1 != 0;
        if (op == G_SETTIMG) {
            pending = (const u8*)g->words.w1;
            format = (g->words.w0 >> 21) & 7;
        }
        if (op == G_SETTILESIZE) {
            int width = ((g->words.w1 >> 12) & 0xfff) / 4 + 1;
            int height = (g->words.w1 & 0xfff) / 4 + 1;
            for (texture = 0; texture < count; ++texture) if (textures[texture].pixels == pending) break;
            if (texture == count) {
                assert(count < 128);
                textures[count++] = (struct Texture){ pending, width, height, format, pending == (const u8*)sSwordIcon };
            }
        }
        if (op != G_TEXRECT) continue;
        assert(texture >= 0 && texture < count);
        fprintf(output, "%s{\"texture\":%d,\"color\":%u,\"gray\":%d,\"box4\":[%u,%u,%u,%u],\"uv\":[%u,%u,%u,%u]}",
                first ? "" : ",", texture, color, gray,
                (unsigned)((g->words.w1 >> 12) & 0xfff), (unsigned)(g->words.w1 & 0xfff),
                (unsigned)((g->words.w0 >> 12) & 0xfff), (unsigned)(g->words.w0 & 0xfff),
                (unsigned)((g[1].words.w1 >> 16) & 0xffff), (unsigned)(g[1].words.w1 & 0xffff),
                (unsigned)((g[2].words.w1 >> 16) & 0xffff), (unsigned)(g[2].words.w1 & 0xffff));
        first = 0;
    }
    fprintf(output, "],\"textures\":[");
    for (int i = 0; i < count; ++i) {
        struct Texture t = textures[i];
        fprintf(output, "%s{\"index\":%d,\"width\":%d,\"height\":%d,\"format\":%d,\"native\":%d}",
                i ? "," : "", i, t.width, t.height, t.format, t.native);
        if (t.native) continue;
        snprintf(path, sizeof(path), "%s-t%02d.bin", prefix, i);
        FILE* pixels = fopen(path, "wb");
        assert(pixels);
        const int bytes = t.width * t.height * (t.format == G_IM_FMT_RGBA ? 4 : 1);
        assert(fwrite(t.pixels, 1, bytes, pixels) == bytes);
        assert(!fclose(pixels));
    }
    fprintf(output, "]}");
    assert(!fclose(output));
}
