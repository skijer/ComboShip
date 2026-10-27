for (unsigned frame : {0u, 47u, 100000u, 0xffffffffu}) {
    for (int effect = 0; effect < 6; ++effect) {
        Reset(frame);
        const auto camera = NeiGi_CameraBasis(&play);
        const auto mesh = effect == 0 ? ForestGiParticles::Saria(frame, false, camera)
                        : effect == 1 ? ForestGiParticles::Saria(frame, true, camera)
                        : effect == 2 ? ForestGiParticles::Mystery(frame, camera)
                        : effect == 3 ? ForestGiParticles::Portal(frame, true, camera)
                        : TempleSagesZeldaAura::Sample(effect == 4 ? 0x7D23 : 0x7F34, frame, camera);
        // ExpandedVertices starts with incoming grayscale enabled. The shared
        // shimmer boundary must clear it and restore the current material pose.
        transform = {5, 12, -8, 2};
        NeiGi_DrawWorldParticles(&play, mesh);
        const auto actual = ExpandedVertices(false);
        assert(actual.size() == mesh.count && resourceLoads == 0);
        for (size_t i = 0; i < actual.size(); ++i) {
            const auto& v = mesh.vertices[i];
            assert(actual[i].v.ob[0] == std::lround(v.p.x * 16));
            assert(actual[i].v.ob[1] == std::lround(v.p.y * 16));
            assert(actual[i].v.ob[2] == std::lround(v.p.z * 16));
            assert(actual[i].v.cn[0] == (v.rgb >> 16));
            assert(actual[i].v.cn[1] == ((v.rgb >> 8) & 255));
            assert(actual[i].v.cn[2] == (v.rgb & 255));
            assert(actual[i].v.cn[3] == v.alpha);
        }
        assert(depth == 0 && stack.empty() && transform.scale == 2);
        assert(transform.x == 5 && transform.y == 12 && transform.z == -8);
        uintptr_t prim = 0, env = 0;
        for (Gfx* command = xlu; command != gfx.polyXlu.p; ++command) {
            if ((command->words.w0 >> 24) == G_SETPRIMCOLOR) prim = command->words.w1;
            if ((command->words.w0 >> 24) == G_SETENVCOLOR) env = command->words.w1;
        }
        assert(prim == 0xffffffff && env == 0xffffffff);
        Reset(frame); allocationFailure = true;
        NeiGi_DrawWorldParticles(&play, mesh);
        assert(vertexLoads.empty() && depth == 0 && stack.empty()); allocationFailure = false;
        for (int remaining = 0; remaining < 64; ++remaining) {
            Reset(frame); gfx.polyOpa.d = opa + remaining; gfx.polyXlu.d = xlu + remaining;
            NeiGi_DrawWorldParticles(&play, mesh);
            assert(gfx.polyOpa.p <= gfx.polyOpa.d && gfx.polyXlu.p <= gfx.polyXlu.d);
            assert(depth == 0 && stack.empty());
        }
    }
}
puts("PASS forest and Zelda aura through actual native GI renderer: exact triangle vertices/palettes/alpha, grayscale boundary, restored colors/matrices and exhausted-arena guards");
