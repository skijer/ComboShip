/**
 * Native put-away bridge for items whose in-hand state also lives outside Player.
 * Included after the item implementations in the player unity build so cleanup
 * uses the same camera, collider, trail and sound teardown as ordinary cancellation.
 */

void CustomItems_CleanupTransientTools(Player* p, PlayState* play) {
    if (p == NULL || play == NULL || p != GET_PLAYER(play)) {
        return;
    }

    s32 blocked = CustomItems_IsBlocked(p, play) || play->csCtx.state != CS_STATE_IDLE;
    if (dlActive &&
        (blocked || !IsItemEquipped(ITEM_DEKU_LEAF) ||
         (dlBlowing && dlAnimTimer > 0 &&
          (p->heldItemAction != PLAYER_IA_DEKU_LEAF || p->upperActionFunc != Player_UpperAction_DekuLeaf)))) {
        DekuLeaf_Stop(p, play);
    }
    if (shActive && (blocked || !IsItemEquipped(ITEM_SHOVEL) ||
                     (shAnimTimer > 0 &&
                      (p->heldItemAction != PLAYER_IA_SHOVEL || p->upperActionFunc != Player_UpperAction_Shovel)))) {
        Shovel_Stop(p, play);
    }
}

void CustomItems_ResetTransientTools(Player* p, PlayState* play) {
    if (p == NULL || play == NULL || p != GET_PLAYER(play)) {
        return;
    }

    // These globals outlive Player/PlayState on an in-process reset or scene load.
    HGrace_ResetTransient();
    DekuLeaf_Stop(p, play);
    Shovel_Stop(p, play);
    if (sDekuLeafColInitialized) {
        Collider_DestroyCylinder(play, &dlCollider);
        sDekuLeafColInitialized = 0;
    }
    sDekuLeafPrevInvinc = sShovelPrevInvinc = 0;
}

static u8 CustomItems_CanStowWhip(void) {
    return whipActive && whipState != WHIP_STATE_SWINGING && whipState != WHIP_STATE_LAUNCHED;
}

s32 CustomItems_HasStowableHeldItem(Player* p) {
    if (p == NULL) {
        return false;
    }
    return gCustomItemState.lanternEquipped || gCustomItemState.lanternSwinging || fireRodActive ||
           fireRodFirstPerson || iceRodActive || iceRodFirstPerson || lightRodActive || lightRodFirstPerson ||
           gCustomItemState.gustJarEquipped || gCustomItemState.mogmaMittsActive ||
           gCustomItemState.ballAndChainThrown || CustomItems_CanStowWhip() || Seasons_IsDrawn();
}

void CustomItems_PutAwayHeldItems(Player* p, PlayState* play) {
    if (p == NULL || play == NULL || p != GET_PLAYER(play)) {
        return;
    }

    Seasons_Stow(play, p);
    Lantern_PutAway(p, play);
    FireRod_PutAway(p, play);
    IceRod_PutAway(p, play);
    LightRod_PutAway(p, play);

    DekuLeaf_Stop(p, play);
    Shovel_Stop(p, play);

    if (gCustomItemState.gustJarEquipped) {
        GustJar_Unequip(play, p);
    }
    if (gCustomItemState.mogmaMittsActive || sMittsEquipState.isEquipped) {
        Mitts_OnUnequip(play, p);
        sMittsEquipState.isEquipped = 0;
    }
    if (gCustomItemState.ballAndChainThrown) {
        ItemInput_SuppressUntilRelease(ITEM_BALL_AND_CHAIN, play);
        BallChain_Stop(p, play);
    }
    if (CustomItems_CanStowWhip()) {
        ItemInput_SuppressUntilRelease(ITEM_WHIP, play);
        Whip_Stop(p, play);
    }
}
