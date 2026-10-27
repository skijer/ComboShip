#define SWORD_FINISH_ADAPTER_TEST
#include "finish_host_test.c"
// Native adapter bodies use the engine/host fixture defined above.
#include "finish_adapters_native.inc"
int main(void) {
  Test_FinishAdapters();
  puts("PASS native custom-death adapters: accepted kill entry, rejected hit, "
       "individual parts and progression");
}
