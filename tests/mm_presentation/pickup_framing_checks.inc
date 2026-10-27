// Use the production CustomItem visible receipt pose, native Item0 closeup
// camera values and renderer submissions. The vertex bounds come from every
// serialized OPA/XLU asset, independently of the runtime frame-fit catalog.
const auto receiptPose=FixtureMmReceiptPose();
assert(std::abs(receiptPose.first-.21f)<.00001f && std::abs(receiptPose.second-53.3f)<.00001f);
const auto pickupVertices=FixtureMmPickupVertices();
size_t nativePickupBindings=0;
for(int form:{PLAYER_FORM_HUMAN,PLAYER_FORM_GORON,PLAYER_FORM_ZORA,PLAYER_FORM_DEKU,PLAYER_FORM_FIERCE_DEITY}) {
 const auto& camera=kMmPickupCamera[form];
 const float height=camera[0],normalizedHeight=.5f*(height+68.f);
 const float cameraAt=height+camera[1]*.01f*normalizedHeight;
 const float cameraDistance=camera[2]*.01f*normalizedHeight;
 const float cameraPitch=camera[3]*NeiGi::Tau/360.f;
 const float cameraTan=std::tan(camera[4]*NeiGi::Tau/720.f);
 const float cameraForward=camera[5];
 for(const auto& f:frames) {
  Reset();
  const std::string raw=std::string("__OTR__objects/nei_gi_redesign/")+f.slug+"/gi_dl";
  const std::string shell=std::string("__OTR__objects/nei_gi_redesign/")+f.slug+"/gi_xlu_dl";
  files.insert(raw);if(f.xlu)files.insert(shell);
  FixtureMmReceiptPose(form);
  Actor receipt{};receipt.id=ACTOR_EN_ITEM00;
  receipt.home.rot.x=CustomItem::CALLED_ACTION|CustomItem::GIVE_ITEM_CUTSCENE;
  CwItemDrawInfo recipe{};
  const auto binding=std::find_if(std::begin(kBindings),std::end(kBindings),
      [&](const auto& b){return !std::strcmp(b.slug,f.slug);});
  if(binding!=std::end(kBindings)) {
    ++nativePickupBindings;
    Rando::DrawResolvedItem(binding->id,1,&receipt);
  } else {
    // Author catalog entries lacking a native MM alias still run through the
    // foreign receipt renderer when a combined check awards that OoT item.
    assert(OOT_GetNeiGiDrawInfo(f.slug,&recipe));
    ComboForeignDrawInfoOOT foreign{};foreign.ok=true;
    foreign.drawKind=recipe.drawKind;foreign.count=recipe.dlistCount;foreign.xluStart=recipe.xluStartIndex;
    foreign.scale=recipe.scale;foreign.neiEffect=recipe.neiEffect;
    std::copy_n(recipe.neiEffectCenter,3,foreign.neiEffectCenter);
    for(int i=0;i<recipe.dlistCount;++i)foreign.dls[i]=NeiResource_Route(recipe.dlists[i]);
    selectedForeignInfoMM=&foreign;
    Rando::DrawResolvedItem(RI_COMBO_FOREIGN,1,&receipt);
    selectedForeignInfoMM=nullptr;
  }
  assert(!submitted.empty() && mmShopLegacyDraws==0);
  const auto [s,y]=submitted.front();
  // Project each real serialized vertex, preserving its Y/radius relation.
  // Goron's forward target offset and steep pitch make wide near-side models
  // clip the bottom even when the previous human envelope contains their Y.
  for(int spin=0;spin<360;spin+=3) {
    const float angle=spin*NeiGi::Tau/360.f,c=std::cos(angle),sine=std::sin(angle);
    for(const auto& point:pickupVertices.at(f.slug)) {
      const float z=cameraForward+s*(point[0]*sine+point[2]*c);
      const float dy=y+s*point[1]-cameraAt;
      const float depth=cameraDistance-dy*std::sin(cameraPitch)-z*std::cos(cameraPitch);
      const float screenY=(dy*std::cos(cameraPitch)-z*std::sin(cameraPitch))/(depth*cameraTan);
      const float screenX=s*(point[0]*c-point[2]*sine)/(depth*cameraTan*(4.f/3.f));
      if(!(depth>0 && screenY<1.f && screenY>-1.f))
        std::cerr<<"MM receipt clips: form="<<form<<" "<<f.slug<<" spin="<<spin<<" projectedY="<<screenY<<"\n";
      assert(depth>0 && screenY<1.f && screenY>-1.f);
      assert(screenX<1.f && screenX>-1.f);
    }
    if(form==PLAYER_FORM_HUMAN)for(float localY:{f.low,f.high}) {
      const float z=.5f*s*f.width*c,dy=y+s*localY-cameraAt;
      const float depth=cameraDistance-dy*std::sin(cameraPitch)-z*std::cos(cameraPitch);
      const float screenY=(dy*std::cos(cameraPitch)-z*std::sin(cameraPitch))/(depth*cameraTan);
      assert(depth>0 && screenY<.9f && screenY>-.9f);
    }
  }
  assert(stack.empty() && interpolation==0);
 }
}
assert(nativePickupBindings>=5*50);
std::cout<<"PASS actual MM CustomItem/DrawResolvedItem receipt route: all "<<std::size(frames)<<" serialized OPA/XLU models inside each of five native form cameras through a full spin; human margin retained\n";
// The selected graph fitter has its own integer C seam. Exercise both active
// custom routes so Goron's context cannot collapse back to the human fit.
for(int form:{PLAYER_FORM_HUMAN,PLAYER_FORM_GORON,PLAYER_FORM_ZORA,PLAYER_FORM_DEKU,PLAYER_FORM_FIERCE_DEITY})
 for(bool foreignReceipt:{false,true}) {
  Reset();ownerAlt=alt=true;
  const char* selected="__OTR__alt/objects/object_custom_equip/gCustomMasterSwordDL";
  files.insert(selected);
  FixtureMmReceiptPose(form);
  Actor receipt{};receipt.id=ACTOR_EN_ITEM00;
  receipt.home.rot.x=CustomItem::CALLED_ACTION|CustomItem::GIVE_ITEM_CUTSCENE;
  if(foreignReceipt) {
    selectedCustomLists=true;
    CwItemDrawInfo owner{};assert(CwAltSwordGi(RG_MASTER_SWORD,&owner));
    ComboForeignDrawInfoOOT foreign{};foreign.ok=true;
    foreign.drawKind=owner.drawKind;foreign.count=owner.dlistCount;foreign.xluStart=owner.xluStartIndex;
    foreign.scale=owner.scale;foreign.neiShimmer=owner.neiShimmer;foreign.itemShimmer=owner.itemShimmer;
    foreign.opCount=owner.opCount;std::copy_n(owner.ops,owner.opCount,foreign.ops);
    foreign.dls[0]=NeiResource_Route(owner.dlists[0]);
    selectedForeignInfoMM=&foreign;
    Rando::DrawResolvedItem(RI_COMBO_FOREIGN,1,&receipt);
    selectedForeignInfoMM=nullptr;
  } else {
    Rando::DrawResolvedItem(RI_OOT_MASTER_SWORD,1,&receipt);
  }
  assert(modelFitContext==(form==PLAYER_FORM_GORON?3:2));
  assert(Drawn()==std::vector<std::string>{std::string("__OTR__@oot:")+(selected+7)});
  assert(stack.empty() && interpolation==0);
 }
ownerAlt=false;
std::cout<<"PASS all five active MM native/foreign selected custom receipt routes retain resource fit contexts\n";
// Generic Poe actor mods must not replace the complete authored Lantern item.
// If either authored pass is missing, retain the legacy mesh and its owning
// manager through the actual native receipt renderer.
for(const char* owner:{"oot","mm"})for(int selectedAlt:{0,1})for(int missing:{-1,0,1}) {
  Reset();alt=selectedAlt;ownerAlt=selectedAlt;
  const char* legacy="__OTR__objects/object_poh/gPoeLanternDL";
  files.insert(legacy);if(selectedAlt)files.insert(std::string("alt/")+legacy);
  const char* authored[]={"__OTR__objects/nei_gi_redesign/lantern/gi_dl",
                          "__OTR__objects/nei_gi_redesign/lantern/gi_xlu_dl"};
  for(int pass=0;pass<2;++pass)if(pass!=missing)files.insert(authored[pass]);
  modFiles.insert({owner,selectedAlt?std::string("alt/")+legacy:std::string(legacy)});
  CwItemDrawInfo selected{};
  assert(MM_DescribeNeiGi(RI_OOT_NEI_LANTERN,&selected));
  const std::vector<std::string> routed=missing<0?std::vector<std::string>{
    "__OTR__@oot:objects/nei_gi_redesign/lantern/gi_dl",
    "__OTR__@oot:objects/nei_gi_redesign/lantern/gi_xlu_dl"}:
    std::vector<std::string>{std::string("__OTR__@")+owner+":objects/object_poh/gPoeLanternDL"};
  assert(selected.drawKind==(missing<0?CW_DRAW_KIND_NEI_GI:CW_DRAW_KIND_CUSTOM_GI) &&
         selected.dlistCount==routed.size() && selected.opCount==0);
  CwItemDrawInfo identity{};assert(OOT_GetNeiGiDrawInfo("lantern",&identity)==(missing<0));
  assert(selected.neiShimmer==identity.neiShimmer && selected.scale==.025f);
  for(size_t pass=0;pass<routed.size();++pass)assert(selected.dlists[pass]==routed[pass]);
  FixtureMmReceiptPose();Actor receipt{};receipt.id=ACTOR_EN_ITEM00;
  receipt.home.rot.x=CustomItem::CALLED_ACTION|CustomItem::GIVE_ITEM_CUTSCENE;
  Rando::DrawResolvedItem(RI_OOT_NEI_LANTERN,1,&receipt);
  assert(Drawn()==routed && mmShopLegacyDraws==0);
  assert(stack.empty() && interpolation==0);
}
ownerAlt=false;
std::cout<<"PASS native MM Lantern authored cage/glass receipts and OOT/MM base/Alt missing-pass fallback ownership\n";
