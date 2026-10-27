/* Compiled after the production declarations and function bodies by the runner. */
static void reset_scene(void) {
    memset(&play, 0, sizeof(play));
    memset(&sLightsBuffer, 0, sizeof(sLightsBuffer));
}

static void init_lit_effect(EffectShieldParticle* effect) {
    EffectShieldParticleInit init = { 0 };
    init.numElements = 1;
    init.lightDecay = true;
    init.lightPoint.radius = 160;
    EffectShieldParticle_Init(effect, &init);
}

static int count_lights(void) {
    int count = 0;
    LightNode* previous = NULL;
    for (LightNode* node = play.lightCtx.listHead; node; node = node->next) {
        assert(node->prev == previous);
        assert(node->info != NULL);
        previous = node;
        assert(++count <= LIGHTS_BUFFER_SIZE);
    }
    assert(count == sLightsBuffer.numOccupied);
    return count;
}

int main(int argc, char** argv) {
    assert(argc == 2);
    reset_scene();
    EffectShieldParticle effect = { 0 };
    LightInfo unrelated[LIGHTS_BUFFER_SIZE] = { 0 };
    if (strcmp(argv[1], "null_light") == 0) {
        /* Exact dump condition: lightDecay=1, lightNode=NULL, listHead=NULL. */
        effect.lightDecay = true;
        EffectShieldParticle_Destroy(&effect);
        EffectShieldParticle_Destroy(NULL);
        assert(count_lights() == 0);
    } else if (strcmp(argv[1], "allocation_failure") == 0) {
        for (int i = 0; i < LIGHTS_BUFFER_SIZE; ++i) {
            assert(LightContext_InsertLight(&play, &play.lightCtx, &unrelated[i]));
        }
        init_lit_effect(&effect);
        assert(effect.lightNode == NULL);
        assert(count_lights() == LIGHTS_BUFFER_SIZE);
        EffectShieldParticle_Destroy(&effect);
        assert(count_lights() == LIGHTS_BUFFER_SIZE);
        while (play.lightCtx.listHead) {
            LightContext_RemoveLight(&play, &play.lightCtx, play.lightCtx.listHead);
        }
        /* Scene teardown revisits inactive slots after other lights are gone. */
        EffectShieldParticle_Destroy(&effect);
        assert(count_lights() == 0);
    } else if (strcmp(argv[1], "head_middle_tail") == 0) {
        for (int position = 0; position < 3; ++position) {
            reset_scene();
            if (position != 2) LightContext_InsertLight(&play, &play.lightCtx, &unrelated[0]);
            init_lit_effect(&effect);
            if (position != 0) LightContext_InsertLight(&play, &play.lightCtx, &unrelated[1]);
            int before = count_lights();
            EffectShieldParticle_Destroy(&effect);
            assert(count_lights() == before - 1);
            assert(effect.lightNode == NULL);
        }
    } else if (strcmp(argv[1], "repeat_destroy") == 0) {
        init_lit_effect(&effect);
        assert(count_lights() == 1);
        EffectShieldParticle_Destroy(&effect);
        EffectShieldParticle_Destroy(&effect);
        assert(count_lights() == 0);
    } else if (strcmp(argv[1], "next_scene") == 0) {
        init_lit_effect(&effect);
        EffectShieldParticle_Destroy(&effect);
        reset_scene();
        LightNode* new_light = LightContext_InsertLight(&play, &play.lightCtx, &unrelated[0]);
        assert(new_light && count_lights() == 1);
        /* Effect_DeleteAll also visits old inactive effect slots in a later scene. */
        EffectShieldParticle_Destroy(&effect);
        assert(count_lights() == 1);
        assert(play.lightCtx.listHead == new_light && new_light->info == &unrelated[0]);
    } else {
        return 2;
    }
    return 0;
}
