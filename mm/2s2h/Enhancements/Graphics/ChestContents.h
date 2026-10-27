#ifndef MM_CHEST_CONTENTS_H
#define MM_CHEST_CONTENTS_H

struct EnBox;
struct PlayState;

#ifdef __cplusplus
extern "C" {
#endif

// Refresh resource names for this draw and return a visual-only scale multiplier.
float MMChest_PrepareDraw(struct EnBox* chest, struct PlayState* play);
// -1 means an unshuffled/native chest; otherwise this is a RandoItemType.
int MMChest_GetRandoItemType(struct EnBox* chest);

#ifdef __cplusplus
}
#endif

#endif
