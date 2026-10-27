// Execute the full native and shared authored renderer after its actual fit.
for (float shelf : { .25f, .32f }) for (bool shared : { false, true }) {
  Reset();
  GetItemEntry capturedEntry{};
  capturedEntry.drawFunc = Randomizer_DrawSwitchHook;
  const auto* binding = FindPresentation(&capturedEntry);
  assert(binding);
  files.insert(binding->opaque);
  const auto* bounds = NeiGi::FindFrameBounds(binding->opaque);
  assert(bounds && bounds->minimum.x + bounds->maximum.x != 0);
  matrix = shelf; matrixY = 37.f; enabled = 1;
  BlackMarketVisual::Capture capture;
  if (shared) {
    const float center[3]{};
    NeiGi_DrawPresentation(&play,binding->opaque,binding->translucent,binding->scale,
                          int(binding->effect),center,true,nullptr,true);
  } else assert(NeiGi_DrawShop(&play,&capturedEntry));
  const auto fit = NeiGi::FrameFit(*bounds,binding->scale,true);
  const float scale = shelf * fit.scale * binding->scale;
  assert(capture.frame.valid);
  assert(std::abs((capture.frame.minimum.x + capture.frame.maximum.x) * .5f -
                  scale * (bounds->minimum.x + bounds->maximum.x) * .5f) < .0001f);
  assert(std::abs((capture.frame.minimum.y + capture.frame.maximum.y) * .5f -
                  (37 + shelf * fit.lift + scale * (bounds->minimum.y + bounds->maximum.y) * .5f)) < .0001f);
  assert(stack.empty() && matrix == shelf && matrixY == 37);
}
for (int season = 1; season <= 4; ++season) {
  Reset(); BlackMarketVisual::Capture capture;
  NeiGi_DrawSeasonOverlay(&play,season,nullptr);
  assert(capture.frame.valid && !arena.empty());
}
{
  Reset(); BlackMarketVisual::Capture capture;
  NeiGi_DrawMesh(&play,NeiGi::SampleShimmer(42,true));
  assert(!capture.frame.valid && !arena.empty());
}
Reset();
std::cout << "PASS authored capture: actual native/shared fit, .25/.32, asymmetric center, seasons only, shimmer excluded\n";
