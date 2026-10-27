// The real foreign dispatchers and portable note handlers emit GPU commands;
// unrelated model families remain at their pre-existing fixture boundaries.
{
    struct Song {int rg,profile;uint32_t hue;};
    const Song songs[]={
        {RG_MINUET_OF_FOREST,CW_SONG_OOT_MINUET,0x62FF62},
        {RG_BOLERO_OF_FIRE,CW_SONG_OOT_BOLERO,0xFF3C00},
        {RG_SERENADE_OF_WATER,CW_SONG_OOT_SERENADE,0x55B4DF},
        {RG_REQUIEM_OF_SPIRIT,CW_SONG_OOT_REQUIEM,0xDE9E2F},
        {RG_NOCTURNE_OF_SHADOW,CW_SONG_OOT_NOCTURNE,0xA028D2},
        {RG_PRELUDE_OF_LIGHT,CW_SONG_OOT_PRELUDE,0xEDE73E},
        {RG_EPONAS_SONG,CW_SONG_OOT_EPONA,0xD96E30},
        {RG_SUNS_SONG,CW_SONG_SUN,0xEDE73E},
        {RG_SONG_OF_STORMS,CW_SONG_STORMS,0x929292},
        {RG_ZELDAS_LULLABY,CW_SONG_OOT_ZELDA,0x6D498F},
        {RG_SARIAS_SONG,CW_SONG_OOT_SARIA,0x3E6D17},
        {RG_SONG_OF_TIME,CW_SONG_TIME,0x62B1D3}};
    for(const auto& song:songs)for(int effects:{0,1})for(int selectedAlt:{0,1}) {
        Reset();enabled=effects;alt=selectedAlt;ownerAlt=selectedAlt;
        GetItemEntry entry{};entry.tableId=TABLE_RANDOMIZER;entry.drawItemId=song.rg;
        CwItemDrawInfo raw{};assert(NeiGi_DescribeEntry(&entry,&raw));
        const uint8_t color[]={uint8_t(song.hue>>16),uint8_t(song.hue>>8),uint8_t(song.hue),255};
        std::vector<std::vector<Vtx>> expected;
        if((song.profile>=CW_SONG_OOT_MINUET && song.profile<=CW_SONG_OOT_PRELUDE) ||
           song.profile==CW_SONG_OOT_EPONA || song.profile==CW_SONG_SUN) {
            NeiGi_DrawShimmerOverlay(&play,color,nullptr);expected=arena;
        } else if(song.profile==CW_SONG_STORMS) {
            NeiGi_DrawMesh(&play,NeiGi::SampleSeason(42,1,NeiGi_CameraBasis(&play)));expected=arena;
        }
        Reset();enabled=effects;alt=selectedAlt;
        ComboForeignDrawInfoOOT info{};info.ok=true;info.count=raw.dlistCount;info.xluStart=raw.xluStartIndex;
        info.drawKind=raw.drawKind;info.itemShimmer=raw.itemShimmer;info.neiEffect=raw.neiEffect;
        std::memcpy(info.itemShimmerColor,raw.itemShimmerColor,4);std::memcpy(info.primColorXlu,raw.primColorXlu,4);
        for(int i=0;i<raw.dlistCount;++i)info.dls[i]=NeiResource_Route(raw.dlists[i]);
        std::vector<std::string> wanted;for(int i=0;i<info.count;++i)wanted.push_back(info.dls[i]);
        portableSongLists=true;
        if(info.count==2)gSPGrayscale(gfx.polyXlu.p++,true);
        selectedForeignInfoMM=&info;MM_DrawComboForeign(1);
        assert(Drawn()==wanted && stack.empty() && "OoT-to-MM route must submit every native warp color list and the clef");
        bool gray=info.count==2;
        uint32_t grayscaleColor=0;
        for(Gfx* cmd=xlu;cmd<gfx.polyXlu.p;++cmd) {
            if(cmd->words.w0>>24==G_SETGRAYSCALE)gray=cmd->words.w1;
            if(cmd->words.w0>>24==G_SETINTENSITY)grayscaleColor=cmd->words.w1;
            if(cmd->words.w0>>24==G_DL_OTR_FILEPATH &&
               !strcmp(reinterpret_cast<const char*>(cmd->words.w1),info.dls[info.count-1])) {
                if(info.count==2)
                    assert(!gray && "warp color material must clear inherited host grayscale before drawing its native clef");
                else
                    assert(gray && grayscaleColor==(song.hue<<8|255u) && "foreign OoT clefs must retain their requested hue");
            }
        }
        if(!expected.empty()) {
            size_t copies=0;
            for(const auto& packed:arena)if(packed.size()==expected.front().size() &&
                !memcmp(packed.data(),expected.front().data(),packed.size()*sizeof(Vtx)))++copies;
            assert(copies==1 && "foreign songs must render their required shared shimmer or rain exactly once");
        }
        if(song.profile==CW_SONG_OOT_EPONA || song.profile==CW_SONG_SUN || song.profile==CW_SONG_STORMS)assert(arena.size()==1);
        if(song.profile==CW_SONG_TIME)assert(arena.empty());
        if(song.profile==CW_SONG_OOT_BOLERO)ExpectBoleroShimmerOnly();
    }
    // MM-owner recipes retain native env tint in OoT. The foreign wrapper
    // restores the incoming pose before rendering the matching song overlay.
    struct MmSong {int profile;uint32_t hue;bool overlay;};
    const MmSong mmSongs[]={
        {CW_SONG_DOUBLE_TIME,0x80D8F0,true}, {CW_SONG_ELEGY,0xFF6200,true},
        {CW_SONG_EPONA,0xD96E30,true}, {CW_SONG_HEALING,0xFF96E6,true},
        {CW_SONG_INVERTED_TIME,0x4A70CA,true}, {CW_SONG_LULLABY_INTRO,0xFF6464,true},
        {CW_SONG_LULLABY,0xFF1414,true}, {CW_SONG_NOVA,0x1414FF,true},
        {CW_SONG_OATH,0x620062,true}, {CW_SONG_SARIA,0x6ACB62,true},
        {CW_SONG_SOARING,0xC8A0FF,true}, {CW_SONG_SONATA,0x62FF62,true},
        {CW_SONG_STORMS,0x929292,true}, {CW_SONG_SUN,0xEDE73E,true},
        {CW_SONG_TIME,0x62B1D3,false},
        {CW_SONG_OOT_MINUET,0x62FF62,true}, {CW_SONG_OOT_BOLERO,0xFF3C00,true},
        {CW_SONG_OOT_SERENADE,0x55B4DF,true}, {CW_SONG_OOT_REQUIEM,0xDE9E2F,true},
        {CW_SONG_OOT_NOCTURNE,0xA028D2,true}, {CW_SONG_OOT_PRELUDE,0xEDE73E,true},
        {CW_SONG_OOT_ZELDA,0x6D498F,true}, {CW_SONG_OOT_EPONA,0xD96E30,true},
        {CW_SONG_OOT_SARIA,0x3E6D17,true}};
    static_assert(sizeof(mmSongs)/sizeof(mmSongs[0])==CW_SONG_COUNT);
    for(const auto& song:mmSongs)for(int effects:{0,1}) {
        Reset();enabled=effects;
        const uint8_t color[]={uint8_t(song.hue>>16),uint8_t(song.hue>>8),uint8_t(song.hue),255};
        NeiGi_DrawSongOverlay(&play,song.profile,nullptr);
        const auto expected=arena;
        Reset();enabled=effects;
        ComboForeignDrawInfo info{};info.ok=true;info.count=1;info.xluStart=0;
        info.drawKind=CW_DRAW_KIND_SONG_GI;info.neiEffect=song.profile;
        info.itemShimmer=song.overlay;info.hasEnvColor=true;
        std::memcpy(info.envColor,color,4);std::memcpy(info.itemShimmerColor,color,4);
        info.dls[0]="__OTR__@mm:objects/object_gi_melody/gGiSongNoteDL";
        portableSongLists=true;
        gSPGrayscale(gfx.polyXlu.p++,true);
        GetItemEntry foreignEntry{};foreignEntry.comboForeignCheck=1;
        selectedForeignInfo=&info;OOT_DrawComboForeign(&play,&foreignEntry);
        assert(Drawn()==std::vector<std::string>{info.dls[0]} && arena.size()==expected.size() && stack.empty());
        bool gray=true;
        uint32_t envColor=0;
        for(Gfx* cmd=xlu;cmd<gfx.polyXlu.p;++cmd) {
            if(cmd->words.w0>>24==G_SETGRAYSCALE)gray=cmd->words.w1;
            if(cmd->words.w0>>24==G_SETENVCOLOR)envColor=cmd->words.w1;
            if(cmd->words.w0>>24==G_DL_OTR_FILEPATH &&
               !strcmp(reinterpret_cast<const char*>(cmd->words.w1),info.dls[0])) {
                assert(!gray && "MM notes must render with their native env color rather than inherited OoT grayscale");
                assert(envColor==(song.hue<<8|255u) && "foreign MM notes must submit their original or explicitly requested hue");
            }
        }
        for(size_t i=0;i<expected.size();++i)assert(arena[i].size()==expected[i].size() &&
              !memcmp(arena[i].data(),expected[i].data(),expected[i].size()*sizeof(Vtx)));
        assert(song.overlay || arena.empty());
        if(song.profile==CW_SONG_OOT_BOLERO)ExpectBoleroShimmerOnly();
    }
    ownerAlt=false;selectedForeignInfo=nullptr;selectedForeignInfoMM=nullptr;
    std::cout<<"PASS both real foreign song dispatchers: all MM profiles, native note/color-list submission, one matching mandatory shimmer, Storms rain only and normal regular songs\n";
}
