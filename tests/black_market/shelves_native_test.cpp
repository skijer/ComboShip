#include "global.h"
#include <array>
#include <cassert>
#include <cmath>
#include <cstdio>
#include <cstring>
#define osSyncPrintf(...) ((void)0)
uintptr_t gSegments[16]{};
static int vertexLoads;
extern "C" void gSPVertex(Gfx* packet,uintptr_t vertices,int count,int start) {
    assert(vertices && count==24 && start==0);++vertexLoads;
    __gSPVertex(packet,vertices,count,start);
}
/* NATIVE COLLISION */
#include "combo/BlackMarketShelvesNative.h"
#include "combo/BlackMarketMmShopLayout.h"
int main() {
    static PlayState play;
    static CollisionHeader scene;
    BlackMarketShelvesNative::Fixture fixture;
    assert(!fixture.Ensure(&play));
    play.colCtx.colHeader=&scene;
    assert(fixture.Ensure(&play));
    const auto& bg=play.colCtx.dyna.bgActors[0];
    assert(bg.actor && bg.actor->update && bg.actor->scale.x==1 && bg.actor->scale.y==1 && bg.actor->scale.z==1);
    assert(reinterpret_cast<DynaPolyActor*>(bg.actor)->bgId==0);
    const auto* mesh=bg.colHeader;
    assert(mesh && mesh->numVertices==16 && mesh->numPolygons==24 && mesh->surfaceTypeList);
    for (int i=0;i<mesh->numVertices;++i) {
        const auto& v=mesh->vtxList[i];
        assert(mesh->minBounds.x<=v.x && v.x<=mesh->maxBounds.x);
        assert(mesh->minBounds.y<=v.y && v.y<=mesh->maxBounds.y);
        assert(mesh->minBounds.z<=v.z && v.z<=mesh->maxBounds.z);
    }
    // Native cameras query dynamic floors too, including the shelf crowns.
    // Exercise the real surface-index and camera lookup without a null guard.
    const auto cameraIndex=SHELF_CAMERA_INDEX(&play.colCtx,&mesh->polyList[0],0);
    assert(cameraIndex==0 && BgCheck_GetBgCamSettingImpl(&play.colCtx,cameraIndex,0)==CAM_SET_NONE);
    for (int index=0;index<24;++index) {
        const auto& poly=mesh->polyList[index];
        const int bank=index/12;
        const auto& box=BlackMarketShelves::kCollision[bank];
        for (int i=0;i<3;++i) {
            assert(poly.vtxData[i]<16);
            const auto& v=mesh->vtxList[poly.vtxData[i]];
            const float plane=(poly.normal.x*v.x+poly.normal.y*v.y+poly.normal.z*v.z)/32767.f+poly.dist;
            // CollisionPoly stores a rounded integer plane distance.
            assert(std::abs(plane)<.51f);
        }
        float cx=0,cy=0,cz=0;
        for (int i=0;i<8;++i) {
            const auto& v=mesh->vtxList[bank*8+i];
            cx+=v.x/8.f;cy+=v.y/8.f;cz+=v.z/8.f;
        }
        assert((poly.normal.x*cx+poly.normal.y*cy+poly.normal.z*cz)/32767.f+poly.dist<0);
    }
    assert(fixture.Ensure(&play) && play.colCtx.dyna.bgActors[1].actor==nullptr);
    std::array<Gfx,2048> commands{};Gfx* head=commands.data();fixture.Draw(head);
    assert(vertexLoads==12 && head<commands.data()+commands.size());
    int triangles=0;bool texture=false,flow=false,frameDrift=false,veilDrift=false;uintptr_t image=0;uint32_t geometry=G_LIGHTING|G_FOG|G_CULL_BOTH;
    for(auto* command=commands.data();command<head;++command) {
        const uint32_t op=uint32_t(command->words.w0)>>24;
        if(op==G_GEOMETRYMODE)geometry=(geometry&(command->words.w0&0xffffff))|command->words.w1;
        if(op==G_TEXTURE)texture=(command->words.w0&255)!=0;
        if(op==G_SETTIMG) {
            image=command->words.w1;
            assert(((command->words.w0>>21)&7)==G_IM_FMT_RGBA && "ethereal shelves need the full violet/cyan color field");
        }
        if(op==G_SCROLL_TEXTURE && command->words.w1)flow=true;
        if(op==G_SCROLL_TEXTURE) {
            const int x=int16_t(command->words.w1>>16),y=int16_t(command->words.w1);
            frameDrift|=x==-1 && y==1;
            veilDrift|=x==1 && y==0;
        }
        if(op==G_SETCOMBINE)assert(((command->words.w0>>5)&15)==G_CCMUX_COMBINED &&
            (command->words.w0&31)==G_CCMUX_PRIMITIVE &&
            "second cycle tints the combined result without borrowing a stale tile-1 texture");
        if(op==G_TRI2)triangles+=2;
        if(op==G_VTX)assert(texture && !(geometry&(G_LIGHTING|G_FOG|G_CULL_BOTH)) && (geometry&G_ZBUFFER));
    }
    assert(triangles==144 && image==uintptr_t(BlackMarketShelves::kFlowTexture));
    assert(flow && "magical shelves need their own scrolling material");
    assert(frameDrift && veilDrift && "frame and quieter veil must drift independently");
    const std::array<uint8_t,4> custom{{220,40,130,0}};
    assert(BlackMarketShelves::Tint(false,custom)==BlackMarketShelves::kDefaultTint);
    assert((BlackMarketShelves::Tint(true,custom)==std::array<uint8_t,4>{220,40,130,255}));
    head=commands.data();fixture.Draw(head,false,{},custom);
    bool tinted=false;
    for (auto* command=commands.data();command<head;++command)
        if ((command->words.w0>>24)==G_SETPRIMCOLOR) tinted=command->words.w1==0xdc2882ff;
    assert(tinted && "cosmetic edits must reach the emitted shelf material");
    static const char wood[]="__OTR__@oot:alt/scenes/shared/mahouya_scene/sh_maho_ki_01_2CA00";
    static const char atlas[]="__OTR__@oot:alt/scenes/shared/mahouya_scene/sh_maho_obj_01_43580";
    head=commands.data();fixture.Draw(head,true,{wood,atlas},custom);
    int woodFaces=0,atlasFaces=0;image=0;bool white=false;
    for (auto* command=commands.data();command<head;++command) {
        const auto op=command->words.w0>>24;
        if(op==G_SETTIMG)image=command->words.w1;
        if(op==G_SETPRIMCOLOR)white=command->words.w1==0xffffffff;
        if(op==G_SCROLL_TEXTURE)assert(command->words.w1==0 && "Granny finishes must stay still");
        if(op==G_TRI2) {
            if(image==uintptr_t(wood))++woodFaces;
            else if(image==uintptr_t(atlas))++atlasFaces;
            else assert(false && "Granny shelves must sample only her external room textures");
        }
    }
    assert(white && woodFaces>0 && atlasFaces>0 && woodFaces+atlasFaces==72);
    assert(head<commands.data()+commands.size());
    head=commands.data();fixture.Draw(head,true,{wood,nullptr});assert(head==commands.data());
    const auto& magicVertices=BlackMarketShelves::Vertices(false);
    const auto& grannyVertices=BlackMarketShelves::Vertices(true);
    for(size_t i=0;i<magicVertices.size();++i) {
        assert(std::memcmp(&magicVertices[i].position,&grannyVertices[i].position,sizeof(BlackMarketShelves::Point))==0);
        assert(grannyVertices[i].color[0]==grannyVertices[i].color[1] && grannyVertices[i].color[1]==grannyVertices[i].color[2]);
    }
    // Context teardown frees native registration; persistent mesh/owner storage
    // remains valid and can register in the next visit without borrowing a slot.
    fixture.Reset();play.colCtx.dyna={};
    assert(fixture.Ensure(&play) && play.colCtx.dyna.bgActors[0].colHeader==mesh);
    fixture.Reset();play.colCtx.dyna={};
    for(auto& flags:play.colCtx.dyna.bgActorFlags)flags=1;
    assert(!fixture.Ensure(&play));
    // The private MM display supplies its compact mesh and collision together.
    // The original fixture above remains the default for OoT.
    BlackMarketShelvesNative::Fixture compact(BlackMarketMmShopLayout::Vertices(false),
        BlackMarketMmShopLayout::Vertices(true), BlackMarketMmShopLayout::CollisionCorners());
    play.colCtx.dyna={};
    assert(compact.Ensure(&play));
    const auto* compactMesh=play.colCtx.dyna.bgActors[0].colHeader;
    const auto corners=BlackMarketMmShopLayout::CollisionCorners();
    for(int bank=0;bank<2;++bank)for(int i=0;i<8;++i) {
        const auto& v=compactMesh->vtxList[bank*8+i];
        const auto& expected=corners[bank][i];
        assert(v.x==expected.x && v.y==expected.y && v.z==expected.z);
    }
    for(bool granny:{false,true}) {
        head=commands.data();vertexLoads=0;
        compact.Draw(head,granny,{wood,atlas});
        assert(vertexLoads==12);
        const auto& expected=BlackMarketMmShopLayout::Vertices(granny);
        size_t index=0;
        for(auto* packet=commands.data();packet<head;++packet)if((packet->words.w0>>24)==G_VTX) {
            const auto* vertices=reinterpret_cast<const Vtx*>(packet->words.w1);
            for(int i=0;i<24;++i,++index) {
                const auto& a=vertices[i].v;const auto& b=expected[index];
                assert(a.ob[0]==b.position.x && a.ob[1]==b.position.y && a.ob[2]==b.position.z);
                assert(a.tc[0]==b.s && a.tc[1]==b.t);
            }
        }
        assert(index==expected.size());
    }
    compact.Reset();play.colCtx.dyna={};
    assert(compact.Ensure(&play) && play.colCtx.dyna.bgActors[0].colHeader==compactMesh);
    std::puts("PASS native shelves: real Dyna/camera lookup; immutable identical geometry; scrolling magical material, live/reset tint and uncolored stationary external Granny timber/velvet/trim; missing texture rejection; opaque GBI; re-entry/full-context failure");
}
