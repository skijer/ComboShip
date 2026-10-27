#include <cassert>
#include <iostream>
#include <set>
#include <string>
#include <vector>
using RandoItemId = int;
using RandoCheckId = int;
namespace Rando {
bool gComboDormantGive = false;
namespace StaticData {
std::set<int> Items{1};
std::string GetItemName(int, bool, int) { return "Power of Magic"; }
} // namespace StaticData
namespace MiscBehavior {
std::string BankRewardSourceSuffix(int) { return " (bank reward)"; }
} // namespace MiscBehavior
} // namespace Rando
namespace ComboRando {
struct ForeignItem {};
std::string ShownForeignName(const ForeignItem &, const char *resolved) {
  return resolved;
}
} // namespace ComboRando
struct Toast {
  std::string message, suffix;
};
namespace Notification {
std::vector<Toast> queue;
void Emit(Toast toast) { queue.push_back(toast); }
} // namespace Notification
void NativePickup(RandoItemId randoItemId, RandoCheckId randoCheckId) {
  /* NATIVE_TOAST */
}
void OotForeignPickup() {
  ComboRando::ForeignItem foreign;
  auto *fi = &foreign;
  const char *resolved = "Power of Magic";
  /* OOT_FOREIGN_TOAST */
}
void MmForeignPickup() {
  struct {
    ComboRando::ForeignItem second;
  } entry;
  auto *it = &entry;
  const char *resolved = "Bottle with Blue Potion";
  RandoCheckId rc = 1;
  using Rando::MiscBehavior::BankRewardSourceSuffix;
  /* MM_FOREIGN_TOAST */
}
int main() {
  NativePickup(1, 1);
  assert(Notification::queue.size() == 1);
  assert(Notification::queue.back().message == "You found");
  assert(Notification::queue.back().suffix == "Power of Magic (bank reward)");
  Notification::queue.clear();
#ifdef COMBO_BUILD
  Rando::gComboDormantGive = true;
  NativePickup(1, 1);
  assert(Notification::queue.empty());
  OotForeignPickup();
  assert(Notification::queue.size() == 1);
  assert(Notification::queue.back().message == "You found" &&
         Notification::queue.back().suffix == "Power of Magic");
  Notification::queue.clear();
  MmForeignPickup();
  assert(Notification::queue.size() == 1);
  assert(Notification::queue.back().message == "You found");
  assert(Notification::queue.back().suffix ==
         "Bottle with Blue Potion (bank reward)");
  Notification::queue.clear();
  Rando::gComboDormantGive = false;
#endif
  NativePickup(999, 1);
  assert(Notification::queue.empty());
  std::cout << "PASS native pickup and finder notification; dormant receiver "
               "stays silent\n";
}
