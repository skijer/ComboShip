#pragma once

#ifdef COMBO_BUILD
#ifdef __cplusplus
extern "C" {
#endif
// Queue only after the matching save and portal destination have been loaded.
// Snowhead queues after its one-shot regional spawn has been validated.
void ComboForestArrivalCamera_Queue(int route);
#ifdef __cplusplus
}
#endif
#endif
