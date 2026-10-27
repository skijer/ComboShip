#include "global.h"
#include "mods/nei_save.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

static NeiSaveData sSave;
static int sCloses;
NeiSaveData *Nei_Save(void) { return &sSave; }
void Message_CloseTextbox(PlayState *play) {
  play->msgCtx.msgMode = MSGMODE_TEXT_CLOSING;
  ++sCloses;
}
#ifdef CAPE_TEST_MM
bool Message_ShouldAdvance(PlayState *play) {
  return !!(play->state.input[0].press.button & BTN_A);
}
#define CAPE_CHOICE_ENDTYPE TEXTBOX_ENDTYPE_TWO_CHOICE
#else
u8 Message_ShouldAdvance(PlayState *play) {
  return !!(play->state.input[0].press.button & BTN_A);
}
#define CAPE_CHOICE_ENDTYPE TEXTBOX_ENDTYPE_2_CHOICE
#endif

/* PRODUCTION_CHOICE */
/* PRODUCTION_UPDATE_PREFIX */

int main(void) {
  static PlayState play;
  MessageContext *msg = &play.msgCtx;
  sSave.capeOwned = 1;
  sSave.capeHidden = 0;
  Message_SetCapeVisibilityChoice(1);
  msg->textboxEndType = CAPE_CHOICE_ENDTYPE;
  msg->choiceIndex = 1;
  msg->msgMode = MSGMODE_TEXT_NEXT_MSG;
  play.state.input[0].press.button = BTN_A;
  assert(!Message_HandleCapeVisibilityChoice(&play));
  assert(sSave.capeHidden == 0 && sCloses == 0);
  msg->msgMode = MSGMODE_TEXT_DONE;
  play.state.input[0].press.button = BTN_B;
  assert(Message_HandleCapeVisibilityChoice(&play));
  assert(sSave.capeHidden == 0 && sCloses == 0);
  play.state.input[0].press.button = BTN_A;
  assert(Message_HandleCapeVisibilityChoice(&play));
  assert(sSave.capeHidden == 1 && sSave.capeOwned == 1 && sCloses == 1);
  assert(msg->msgMode == MSGMODE_TEXT_CLOSING);
  assert(!Message_HandleCapeVisibilityChoice(&play) && sCloses == 1);
  Message_SetCapeVisibilityChoice(1);
  msg->msgMode = MSGMODE_TEXT_DONE;
  msg->choiceIndex = 0;
  assert(Message_HandleCapeVisibilityChoice(&play));
  assert(sSave.capeHidden == 0 && sSave.capeOwned == 1 && sCloses == 2);
  Message_SetCapeVisibilityChoice(1);
  Message_SetCapeVisibilityChoice(
      0); // Every new native textbox clears the receipt marker.
  msg->msgMode = MSGMODE_TEXT_DONE;
  msg->choiceIndex = 1;
  assert(!Message_HandleCapeVisibilityChoice(&play));
  assert(sSave.capeHidden == 0 && sCloses == 2);
#ifdef CAPE_TEST_MM
  // Equipment appearance choices use the native pause-description close path.
  // B/Start cancel there without applying the highlighted choice; receipts
  // retain A-only confirmation.
  play.pauseCtx.itemDescriptionOn = 1;
  Message_SetCapeVisibilityChoice(1);
  msg->msgMode = MSGMODE_TEXT_DONE;
  msg->choiceIndex = 1;
  play.state.input[0].press.button = BTN_B;
  assert(Message_HandleCapeVisibilityChoice(&play));
  assert(sSave.capeHidden == 0 && sSave.capeOwned == 1 && sCloses == 3);
  assert(msg->msgMode == MSGMODE_TEXT_CLOSING);
  assert(!Message_HandleCapeVisibilityChoice(&play));
  sSave.capeHidden = 1;
  Message_SetCapeVisibilityChoice(1);
  msg->msgMode = MSGMODE_TEXT_DONE;
  msg->choiceIndex = 0;
  play.state.input[0].press.button = BTN_START;
  assert(Message_HandleCapeVisibilityChoice(&play));
  assert(sSave.capeHidden == 1 && sSave.capeOwned == 1 && sCloses == 4);
  Message_SetCapeVisibilityChoice(1);
  msg->msgMode = MSGMODE_TEXT_DONE;
  play.state.input[0].press.button = BTN_A;
  assert(Message_HandleCapeVisibilityChoice(&play));
  assert(sSave.capeHidden == 0 && sSave.capeOwned == 1 && sCloses == 5);
  // Cancellation must reach the handler before the normal mode dispatcher:
  // the intro/reveal/page-wait stages do not yet have a two-choice end type.
  const u8 popupModes[] = {MSGMODE_TEXT_NEXT_MSG, MSGMODE_TEXT_DISPLAYING,
                           MSGMODE_TEXT_AWAIT_NEXT, MSGMODE_TEXT_DONE};
  const u16 cancelButtons[] = {BTN_B, BTN_START};
  for (unsigned mode = 0; mode < sizeof(popupModes) / sizeof(popupModes[0]);
       ++mode) {
    for (unsigned button = 0;
         button < sizeof(cancelButtons) / sizeof(cancelButtons[0]); ++button) {
      for (u8 hidden = 0; hidden < 2; ++hidden) {
        sSave.capeHidden = hidden;
        const NeiSaveData beforeCancel = sSave;
        const int closes = sCloses;
        Message_SetCapeVisibilityChoice(1);
        msg->msgMode = popupModes[mode];
        msg->textboxEndType = popupModes[mode] == MSGMODE_TEXT_DONE
                                  ? CAPE_CHOICE_ENDTYPE
                                  : TEXTBOX_ENDTYPE_DEFAULT;
        msg->choiceIndex = !hidden;
        play.state.input[0].press.button = cancelButtons[button];
        Message_UpdateCapePrefix(&play);
        assert(sCloses == closes + 1 && msg->msgMode == MSGMODE_TEXT_CLOSING);
        assert(memcmp(&beforeCancel, &sSave, sizeof(sSave)) == 0);
        Message_UpdateCapePrefix(&play);
        assert(
            sCloses ==
            closes +
                1); // Closing disarms the popup instead of closing repeatedly.
      }
    }
  }
  // Early A remains an advance/reveal action, and B/Start on receipts keep
  // their existing behavior instead of applying/canceling an appearance choice.
  const int closes = sCloses;
  Message_SetCapeVisibilityChoice(1);
  msg->msgMode = MSGMODE_TEXT_DISPLAYING;
  msg->textboxEndType = TEXTBOX_ENDTYPE_DEFAULT;
  play.state.input[0].press.button = BTN_A;
  Message_UpdateCapePrefix(&play);
  assert(!Message_HandleCapeVisibilityChoice(&play) && sCloses == closes);
  play.pauseCtx.itemDescriptionOn = 0;
  for (unsigned button = 0;
       button < sizeof(cancelButtons) / sizeof(cancelButtons[0]); ++button) {
    play.state.input[0].press.button = cancelButtons[button];
    Message_UpdateCapePrefix(&play);
    assert(!Message_HandleCapeVisibilityChoice(&play) && sCloses == closes);
    msg->msgMode = MSGMODE_TEXT_DONE;
    msg->textboxEndType = CAPE_CHOICE_ENDTYPE;
    assert(Message_HandleCapeVisibilityChoice(&play) && sCloses == closes);
    msg->msgMode = MSGMODE_TEXT_DISPLAYING;
    msg->textboxEndType = TEXTBOX_ENDTYPE_DEFAULT;
  }
#endif
#ifndef CAPE_TEST_MM
  // Savestate loads restore msgCtx directly, without opening a new textbox.
  unsigned char snapshot[128];
  MessageContext restored = *msg;
  SaveStateCtx save = {snapshot, 0, SHIP_SAVESTATE_SAVE};
  Message_SetCapeVisibilityChoice(1);
  MessagePAL_SaveState(&save);
  assert(Message_HandleCapeVisibilityChoice(&play));
  assert(sSave.capeHidden == 1 && sCloses == 3);
  *msg = restored;
  SaveStateCtx load = {snapshot, 0, SHIP_SAVESTATE_LOAD};
  MessagePAL_SaveState(&load);
  msg->choiceIndex = 0;
  assert(Message_HandleCapeVisibilityChoice(&play));
  assert(sSave.capeHidden == 0 && sCloses == 4);
  // Restoring an ordinary choice must also clear a subsequently armed cape.
  save.offset = 0;
  MessagePAL_SaveState(&save);
  Message_SetCapeVisibilityChoice(1);
  *msg = restored;
  load.offset = 0;
  MessagePAL_SaveState(&load);
  assert(!Message_HandleCapeVisibilityChoice(&play));
  assert(sSave.capeHidden == 0 && sCloses == 4);
#endif
  puts("PASS native cape show/hide, A confirmation, premature input, ownership "
       "and textbox close");
  return 0;
}
