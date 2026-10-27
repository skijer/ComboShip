// TU glue: included once by each native sword-damage module after EnMThunder.
#ifdef CW_SPIN_OWNERSHIP_MM
#define CW_SPIN_HIT_INFO ColliderElement
#define CW_SPIN_HIT_FLAGS(info) ((info)->atDmgInfo.dmgFlags)
#define CW_SPIN_CYLINDER_INFO(spin) (&(spin)->collider.elem)
#define CW_SPIN_ACTORS(play) ((play)->actorCtx.actorLists[ACTORCAT_ITEMACTION].first)
#define CW_SPIN_SCENE(play) ((play)->sceneId)
#else
#define CW_SPIN_HIT_INFO ColliderInfo
#define CW_SPIN_HIT_FLAGS(info) ((info)->toucher.dmgFlags)
#define CW_SPIN_CYLINDER_INFO(spin) (&(spin)->collider.info)
#define CW_SPIN_ACTORS(play) ((play)->actorCtx.actorLists[ACTORCAT_ITEMACTION].head)
#define CW_SPIN_SCENE(play) ((play)->sceneNum)
#endif

uint32_t SpinAttack_OriginalDamageFlags(PlayState* play, const CW_SPIN_HIT_INFO* info) {
    if (info == NULL)
        return 0;
    uint32_t flags = CW_SPIN_HIT_FLAGS(info);
    if (play == NULL || GET_PLAYER(play) == NULL)
        return flags;
    for (Actor* actor = CW_SPIN_ACTORS(play); actor != NULL; actor = actor->next) {
        if (actor->id != ACTOR_EN_M_THUNDER || actor->update == NULL || actor->category != ACTORCAT_ITEMACTION)
            continue;
        EnMThunder* spin = (EnMThunder*)actor;
        if (info != CW_SPIN_CYLINDER_INFO(spin) || spin->collider.base.actor != actor)
            continue;
        const CwSpinAttackState* state = &spin->spinStyle;
        if (state->context != (uintptr_t)play || state->scene != CW_SPIN_SCENE(play) ||
            state->owner != (uintptr_t)GET_PLAYER(play))
            return flags;
        return CwSpinAttack_Original(state, flags);
    }
    return flags;
}

#undef CW_SPIN_HIT_INFO
#undef CW_SPIN_HIT_FLAGS
#undef CW_SPIN_CYLINDER_INFO
#undef CW_SPIN_ACTORS
#undef CW_SPIN_SCENE
