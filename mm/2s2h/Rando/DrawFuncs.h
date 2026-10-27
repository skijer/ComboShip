#ifndef RANDO_DRAW_FUNCS_H
#define RANDO_DRAW_FUNCS_H

#include "Rando/Rando.h"

void DrawEnLight(Color_RGB8 flameColor, Vec3f flameSize);
void DrawMmSoulFlame(PlayState* play, const uint8_t color[3], const float scale[3]);
void DrawOotSoulFlame(PlayState* play, const uint8_t color[3], const float translate[3], const float scale[3]);

// Boss Functions
void DrawGoht();
void DrawGyorg();
void DrawMajora();
void DrawOdolwa();
void DrawTwinmold();

// Enemy Functions
void DrawAlien();
void DrawArmos();
void DrawBat();
void DrawBeamos();
void DrawBoe();
void DrawBubble();
void DrawCaptainKeeta();
void DrawChuchu();
void DrawDeathArmos();
void DrawDeepPython();
void DrawDekuBaba();
void DrawDexihand();
void DrawDinolfos();
void DrawDodongo();
void DrawDragonfly();
void DrawEeno();
void DrawEyegore();
void DrawFlyingPot();
void DrawFreezard();
void DrawGaro();
void DrawGekko();
void DrawGiantBee();
void DrawGomess();
void DrawGuay();
void DrawHiploop();
void DrawIgosDuIkana();
void DrawIronKnuckle();
void DrawKeese();
void DrawLeever();
void DrawLikeLike();
void DrawMadScrub();
void DrawNejiron();
void DrawOctorok();
void DrawPeahat();
void DrawPirate();
void DrawPoe();
void DrawRealBombchu();
void DrawRedead();
void DrawShellBlade();
void DrawSkullfish();
void DrawSkulltula();
void DrawSnapper();
void DrawStalchild();
void DrawTakkuri();
void DrawTektite();
void DrawWallmaster();
void DrawWart();
void DrawWizrobe();
void DrawWolfos();

// Other Actor Functions
void DrawMinifrog(RandoItemId randoItemId, Actor* actor);

// Clock Function
void DrawClock(RandoItemId randoItemId, Actor* actor);

#endif
