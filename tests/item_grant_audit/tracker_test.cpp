#include "ItemGrantAudit.h"
#include <cassert>
#include <iostream>
#include <vector>

using namespace ItemGrantAudit;
int main() {
  Tracker tracker;
  Snapshot save;
  save.file = 0;
  save.mode = 0;
  save.saveType = 1;
  save.seed = 123;
  save.Add("lightArrow", 0xFF, 0, 0xFF);
  save.Add("dins", 0);
  std::vector<std::string> records;
  auto sink = [&](const std::string &line) { records.push_back(line); };
  auto has = [&](const std::string &text) {
    return records.back().find(text) != std::string::npos;
  };
  tracker.Checkpoint(save, "boot", sink);
  assert(has("kind=state-present"));
  const auto first = records.size();
  for (int i = 0; i < 599; ++i)
    tracker.Checkpoint(save, "frame", sink);
  assert(records.size() == first + 1 && has("kind=coverage") &&
         has("changes=0"));
  // A raw write before a legitimate give must not be blamed on the legitimate
  // give.
  save.fields[1].value = 1;
  tracker.Begin(save, "pickup", 42, 7, false, sink);
  assert(records[records.size() - 2].find(
             "origins=unscoped-interval dins[0]=0x0->0x1") !=
         std::string::npos);
  save.fields[0].value = 0x12;
  tracker.End(save, sink);
  assert(has("pickup(42,7)") && has("lightArrow[0]=0xff->0x12"));
  // Preview mutation and nested resolver grants retain the entire active call
  // chain.
  tracker.Begin(save, "preview", -1, -1, true, sink);
  tracker.Begin(save, "resolver", 90, -1, false, sink);
  save.fields[1].value = 3;
  tracker.End(save, sink);
  assert(has("preview(-1,-1)/resolver(90,-1)/") && has("dins[0]=0x1->0x3"));
  tracker.End(save, sink);
  const auto afterPreview = records.size();
  for (int i = 0; i < 1000; ++i) {
    tracker.Begin(save, "preview", -1, -1, true, sink);
    tracker.End(save, sink);
  }
  assert(records.size() == afterPreview); // no unchanged per-draw spam
  // Seed-fill oracle writes are scratch state; restored live state is compared
  // to the live baseline.
  const auto live = save;
  tracker.Suspend(save, sink);
  save.fields[0].value = 0x99;
  tracker.Begin(save, "scratch-grant", 99, 99, false, sink);
  tracker.End(save, sink);
  tracker.Checkpoint(save, "scratch", sink);
  const auto scratch = records.size();
  save = live;
  tracker.Resume(save, sink);
  assert(records.size() == scratch + 1 && has("oracle-live-restored") &&
         has("kind=coverage"));
  tracker.Suspend(save, sink);
  save.fields[1].value = 7; // failed restore must remain observable
  tracker.Resume(save, sink);
  assert(has("oracle-live-restored") && has("dins[0]=0x3->0x7"));
  // Loading another file is state present, not a newly earned grant.
  save.file = 1;
  tracker.Checkpoint(save, "load", sink);
  assert(has("baseline=1") && has("kind=state-present"));
  save.seed = 456;
  tracker.Checkpoint(save, "new-seed-same-slot", sink);
  assert(has("baseline=1"));
  // Removals and nested suspension are visible and balanced.
  tracker.Suspend(save, sink);
  tracker.Suspend(save, sink);
  tracker.Resume(save, sink);
  const auto paused = records.size();
  tracker.Checkpoint(save, "still-paused", sink);
  assert(records.size() == paused);
  tracker.Resume(save, sink);
  tracker.Begin(save, "cycle-reset", -1, -1, false, sink);
  save.fields[0].value = 0xFF;
  tracker.End(save, sink);
  assert(has("lightArrow[0]=0x12->0xff"));
  // Bound over-nested calls without losing unwind balance.
  for (int i = 0; i < 70; ++i)
    tracker.Begin(save, "nested", i, -1, true, sink);
  for (int i = 0; i < 70; ++i)
    tracker.End(save, sink);
  tracker.Checkpoint(save, "balanced", sink);
  tracker.End(save, sink);
  assert(has("ERROR unbalanced scope"));
  Snapshot overflow;
  for (size_t i = 0; i < 2050; ++i)
    overflow.Add("oversized", 1, i);
  tracker.Checkpoint(overflow, "oversized", sink);
  assert(has("overflow=1"));
  Snapshot signedCounters;
  int8_t keys[] = {-1, 0, 3};
  signedCounters.Array("keys", keys, 0xFF);
  assert(signedCounters.fields[0].value == 0xFF);
  std::cout << "provenance tracker: clean run, bypass write, nested preview, "
               "oracle restore, context and bounds PASS\n";
}
