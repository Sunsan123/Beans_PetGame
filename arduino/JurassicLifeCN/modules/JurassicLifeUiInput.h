#pragma once

// Touch, button, and encoder UI input routing.
// Extracted from JurassicLifeCN.ino during the architecture refactor.

// ================== TACTILE -> UI (AJOUT) ==================
struct TouchDeb {
  bool rawDown = false;
  bool stableDown = false;
  uint32_t lastChange = 0;
  int16_t x = 0, y = 0;
  int8_t  lastBtn = -1;
    int8_t  pressBtn = -1;
  uint32_t pressStart = 0;
  bool longPressFired = false;
};
static TouchDeb touch;
static const uint32_t TOUCH_DEBOUNCE_MS = 25;
static const uint32_t AUDIO_LONG_PRESS_MS = 650;

static inline bool readTouchRaw(int16_t &x, int16_t &y) {
  return readTouchScreen(x, y);
}

// ================== Input UI ==================
static void closeInteractionMenu() {
  interactionMenuOpen = false;
  interactionMenuSel = 0;
}

static void toggleInteractionMenu(uint32_t now) {
  (void)now;
  interactionMenuOpen = !interactionMenuOpen;
  if (!interactionMenuOpen) interactionMenuSel = 0;
  uiSpriteDirty = true;
  uiForceBands = true;
}

static void calcInteractionMenuRect(int& menuX, int& menuY, int& menuW, int& menuH) {
  const int menuCount = (int)interactionMenuCount();
  const int itemGap = 4;
  const int itemH = 20;
  int layoutStartX = 0, layoutBw = 0, layoutBh = 0, layoutY = 0, layoutGap = 0;
  calcBottomActionLayout(uiButtonCount(), layoutStartX, layoutBw, layoutBh, layoutY, layoutGap);
  menuW = min(100, SW - 12);
  menuH = menuCount * itemH + max(0, menuCount - 1) * itemGap + 8;
  menuX = max(4, layoutStartX);
  menuY = SH - UI_BOT_H - menuH - 6;
  if (menuY < 70) menuY = 70;
}

static bool performUiAction(UiAction a, uint32_t now) {
  if (a != UI_INTERACT && (int32_t)(now - cdUntil[(int)a]) < 0) {
    uint32_t sec = (cdUntil[(int)a] - now) / 1000UL;
    char tmp[48];
    snprintf(tmp, sizeof(tmp), "\xe8\xae\xbe\xe6\x96\xbd\xe5\x86\xb7\xe5\x8d\xb4(%lus)", (unsigned long)sec);
    setMsg(tmp, now, 1500);
    uiSpriteDirty = true;
    uiForceBands = true;
    return false;
  }

  switch (a) {
    case UI_REPOS:
      cdUntil[a] = now + 1000;
      closeInteractionMenu();
      return startTask(TASK_SLEEP, now);
    case UI_MANGER:
      cdUntil[a] = now + CD_EAT_MS;
      closeInteractionMenu();
      return startTask(TASK_EAT, now);
    case UI_BOIRE:
      cdUntil[a] = now + CD_DRINK_MS;
      closeInteractionMenu();
      return startTask(TASK_DRINK, now);
    case UI_LAVER:
      cdUntil[a] = now + CD_WASH_MS;
      closeInteractionMenu();
      mgBegin(TASK_WASH, now);
      return true;
    case UI_JOUER:
      cdUntil[a] = now + CD_PLAY_MS;
      closeInteractionMenu();
      mgBegin(TASK_PLAY, now);
      return true;
    case UI_CACA:
      cdUntil[a] = now + CD_POOP_MS;
      closeInteractionMenu();
      return startTask(TASK_POOP, now);
    case UI_CALIN:
      cdUntil[a] = now + CD_HUG_MS;
      closeInteractionMenu();
      return startTask(TASK_HUG, now);
    case UI_INTERACT:
      toggleInteractionMenu(now);
      return true;
    case UI_DRESS_MODE:
      closeInteractionMenu();
      setMsg("\xe8\xa3\x85\xe6\x89\xae\xe6\xa8\xa1\xe5\xbc\x8f\xe6\x95\xb4\xe7\x90\x86\xe4\xb8\xad", now, 1600);
      return true;
    case UI_DRESS_SHOP:
      closeInteractionMenu();
      setMsg("\xe8\xa3\x85\xe6\x89\xae\xe5\x95\x86\xe5\xba\x97\xe5\x87\x86\xe5\xa4\x87\xe4\xb8\xad", now, 1600);
      return true;
    case UI_AUDIO:
      closeInteractionMenu();
      if (audioMode == AUDIO_TOTAL) audioMode = AUDIO_LIMITED;
      else if (audioMode == AUDIO_LIMITED) audioMode = AUDIO_OFF;
      else audioMode = AUDIO_TOTAL;

      if (audioMode == AUDIO_OFF) {
        stopAudio();
        setMsg("\xe7\xbb\x88\xe7\xab\xaf\xe9\x9d\x99\xe9\x9f\xb3", now, 1500);
      } else if (audioMode == AUDIO_LIMITED) {
        audioNextAlertAt = now + 20000UL;
        setMsg("\xe7\xbb\x88\xe7\xab\xaf\xe4\xbd\x8e\xe6\x8f\x90\xe7\xa4\xba", now, 1500);
      } else {
        audioNextAlertAt = now + 10000UL;
        setMsg("\xe7\xbb\x88\xe7\xab\xaf\xe5\x85\xa8\xe6\x8f\x90\xe7\xa4\xba", now, 1500);
      }
#if ENABLE_AUDIO
      if (saveReady) saveNow(now, "audio");
#endif
      return true;
    default:
      return false;
  }
}

static void handleEncoderUI(uint32_t now) {
  if (!encoderEnabled()) return;
  int32_t p;
  noInterrupts(); p = encPos; interrupts();
  int32_t det = detentFromEnc(p);
  int32_t dd  = det - lastDetent;

  uint8_t nbtn = uiButtonCount();

  while (dd < 0) {
    if (interactionMenuOpen && uiSel == 0) {
      if (interactionMenuSel > 0) interactionMenuSel--;
    } else if (uiShowSceneArrows() && uiSel == 0) {
      switchSceneByOffset(-1, now);
    } else if (uiSel > 0) {
      if (interactionMenuOpen && uiSel == 1) closeInteractionMenu();
      uiSel--;
    }
    dd++;
    uiSpriteDirty = true;
    uiForceBands  = true;
  }
  while (dd > 0) {
    if (interactionMenuOpen && uiSel == 0) {
      if (interactionMenuSel + 1 < interactionMenuCount()) interactionMenuSel++;
      else if (nbtn > 1) { closeInteractionMenu(); uiSel = 1; }
    } else if (uiShowSceneArrows() && uiSel == (uint8_t)(nbtn - 1)) {
      switchSceneByOffset(+1, now);
    } else if (uiSel + 1 < nbtn) {
      uiSel++;
    }
    dd--;
    lastDetent = det;
    uiSpriteDirty = true;
    uiForceBands  = true;
  }
  if (dd == 0) lastDetent = det;

  bool raw = readBtnPressedRaw();
  if (raw != lastBtnRaw) { lastBtnRaw = raw; btnChangeAt = now; }
  if ((now - btnChangeAt) > BTN_DEBOUNCE_MS) btnState = raw;

  static bool lastStable = false;
  bool pressedEdge = (btnState && !lastStable);
  lastStable = btnState;
  if (!pressedEdge) return;

  uiPressAction(now);
}

static void handleButtonsUI(uint32_t now) {
  if (!buttonsEnabled()) return;
  uint8_t nbtn = uiButtonCount();

  if (btnLeftEdge) {
    if (interactionMenuOpen && uiSel == 0) {
      if (interactionMenuSel > 0) interactionMenuSel--;
    } else if (uiShowSceneArrows() && uiSel == 0) {
      switchSceneByOffset(-1, now);
    } else if (uiSel > 0) {
      if (interactionMenuOpen && uiSel == 1) closeInteractionMenu();
      uiSel--;
    }
    uiSpriteDirty = true;
    uiForceBands  = true;
  }

  if (btnRightEdge) {
    if (interactionMenuOpen && uiSel == 0) {
      if (interactionMenuSel + 1 < interactionMenuCount()) interactionMenuSel++;
      else if (nbtn > 1) { closeInteractionMenu(); uiSel = 1; }
    } else if (uiShowSceneArrows() && uiSel == (uint8_t)(nbtn - 1)) {
      switchSceneByOffset(+1, now);
    } else if (uiSel + 1 < nbtn) {
      uiSel++;
    }
    uiSpriteDirty = true;
    uiForceBands  = true;
  }

  if (btnOkEdge) {
    uiPressAction(now);
  }
}

// --- Action commune (encodeur + tactile) ---
static void uiPressAction(uint32_t now) {
  if (appMode != MODE_PET) return; // en mini-jeu, on ne clique pas l'UI gestion

  if (phase == PHASE_EGG) {
    phase = PHASE_HATCHING;
    hatchIdx = 0;
    hatchNext = now + 220;

    activityVisible = true;
    activityStart = now;
    activityEnd   = now + 1200;
    strcpy(activityText, "\xe7\xbd\x97\xe5\xbe\xb7\xe5\xb2\x9b\xe5\xad\xb5\xe5\x8c\x96\xe4\xb8\xad...");  // 罗德岛孵化中...
    uiSpriteDirty = true; uiForceBands = true;
    return;
  }

    if (phase == PHASE_TOMB) {
    eraseSavesAndRestart();
    return;
  }

  if (phase == PHASE_HATCHING) return;

  if (phase == PHASE_RESTREADY) {
    resetToEgg(now);
    setMsg("\xe6\x96\xb0\xe7\x9a\x84\xe6\x94\xb6\xe5\xae\xb9\xe8\x88\xb1\xe5\xb7\xb2\xe5\xb0\xb1\xe7\xbb\xaa", now, 1800);  // 新的收容舱已就绪
    if (saveReady) saveNow(now, "rest_new");
    return;
  }

  if (state == ST_SLEEP) {
    task.active = false;
    task.kind = TASK_NONE;
    lastSleepGainAt = 0;

    activityVisible = false;
    activityText[0] = 0;

    enterState(ST_SIT, now);
    playRTTTLOnce(RTTTL_SLEEP_INTRO, AUDIO_PRIO_MED);
    setMsg("\xe4\xbc\x91\xe6\x95\xb4\xe7\xbb\x93\xe6\x9d\x9f\xef\xbc\x8c\xe7\xbb\xa7\xe7\xbb\xad\xe5\xb7\xa1\xe6\x88\xbf!", now, 1800);  // 休整结束，继续巡房!
    uiSpriteDirty = true; uiForceBands = true;
    return;
  }

  if (task.active) { setMsg("\xe5\xbd\x93\xe5\x89\x8d\xe6\xad\xa3\xe5\x9c\xa8\xe6\x89\xa7\xe8\xa1\x8c\xe5\xae\x89\xe6\x8e\x92!", now, 1400); uiSpriteDirty = true; uiForceBands = true; return; }  // 当前正在执行安排!

uint8_t nbtn = uiButtonCount();
if (nbtn != uiAliveCount()) return;

  UiAction a = uiAliveActionAt(uiSel);

  if (interactionMenuOpen && uiSel == 0 && a == UI_INTERACT) {
    a = interactionMenuActionAt(interactionMenuSel);
  }

  if (a != UI_INTERACT && (int32_t)(now - cdUntil[(int)a]) < 0) {
    uint32_t sec = (cdUntil[(int)a] - now) / 1000UL;
    char tmp[48];
    snprintf(tmp, sizeof(tmp), "\xe8\xae\xbe\xe6\x96\xbd\xe5\x86\xb7\xe5\x8d\xb4(%lus)", (unsigned long)sec);  // 设施冷却(Xs)
    setMsg(tmp, now, 1500);
    uiSpriteDirty = true; uiForceBands = true;
    return;
  }

  switch (a) {
    case UI_REPOS:
      closeInteractionMenu();
      cdUntil[a] = now + 1000;
      startTask(TASK_SLEEP, now);
      break;

    case UI_MANGER:
      closeInteractionMenu();
      cdUntil[a] = now + CD_EAT_MS;
      startTask(TASK_EAT, now);
      break;

    case UI_BOIRE:
      closeInteractionMenu();
      cdUntil[a] = now + CD_DRINK_MS;
      startTask(TASK_DRINK, now);
      break;

    case UI_LAVER:
      closeInteractionMenu();
      cdUntil[a] = now + CD_WASH_MS;
      mgBegin(TASK_WASH, now);  // <--- MINI-JEU
      break;

    case UI_JOUER:
      closeInteractionMenu();
      cdUntil[a] = now + CD_PLAY_MS;
      mgBegin(TASK_PLAY, now);  // <--- MINI-JEU
      break;

    case UI_CACA:
      closeInteractionMenu();
      cdUntil[a] = now + CD_POOP_MS;
      startTask(TASK_POOP, now);
      break;

    case UI_CALIN:
      closeInteractionMenu();
      cdUntil[a] = now + CD_HUG_MS;
      startTask(TASK_HUG, now);
      break;

    case UI_INTERACT:
      toggleInteractionMenu(now);
      break;

    case UI_DRESS_MODE:
      closeInteractionMenu();
      setMsg("\xe8\xa3\x85\xe6\x89\xae\xe6\xa8\xa1\xe5\xbc\x8f\xe6\x95\xb4\xe7\x90\x86\xe4\xb8\xad", now, 1600);
      break;

    case UI_DRESS_SHOP:
      closeInteractionMenu();
      setMsg("\xe8\xa3\x85\xe6\x89\xae\xe5\x95\x86\xe5\xba\x97\xe5\x87\x86\xe5\xa4\x87\xe4\xb8\xad", now, 1600);
      break;

    case UI_AUDIO: {
      if (audioMode == AUDIO_TOTAL) audioMode = AUDIO_LIMITED;
      else if (audioMode == AUDIO_LIMITED) audioMode = AUDIO_OFF;
      else audioMode = AUDIO_TOTAL;

      if (audioMode == AUDIO_OFF) {
        stopAudio();
        setMsg("\xe7\xbb\x88\xe7\xab\xaf\xe9\x9d\x99\xe9\x9f\xb3", now, 1500);    // 终端静音
      } else if (audioMode == AUDIO_LIMITED) {
        audioNextAlertAt = now + 20000UL;
        setMsg("\xe7\xbb\x88\xe7\xab\xaf\xe4\xbd\x8e\xe6\x8f\x90\xe7\xa4\xba", now, 1500);    // 终端低提示
      } else {
        audioNextAlertAt = now + 10000UL;
        setMsg("\xe7\xbb\x88\xe7\xab\xaf\xe5\x85\xa8\xe6\x8f\x90\xe7\xa4\xba", now, 1500);    // 终端全提示
      }
#if ENABLE_AUDIO
      if (saveReady) saveNow(now, "audio");
#endif
      break;
    }

    default: break;
  }

  uiSpriteDirty = true;
  uiForceBands  = true;
}

static void handleTouchUI(uint32_t now) {
  if (appMode != MODE_PET) return; // en mini-jeu, on gère le tactile dans le mini-jeu si besoin

  int16_t x, y;
  bool down = readTouchRaw(x, y);

  if (down != touch.rawDown) {
    touch.rawDown = down;
    touch.lastChange = now;
    if (down) { touch.x = x; touch.y = y; }
  } else if (down) {
    touch.x = x; touch.y = y;
  }

  bool stableDown = touch.rawDown && (now - touch.lastChange) >= TOUCH_DEBOUNCE_MS;
  bool stableUp   = (!touch.rawDown) && (now - touch.lastChange) >= TOUCH_DEBOUNCE_MS;

  bool pressedEdge  = false;
  bool releasedEdge = false;

  if (!touch.stableDown && stableDown) { touch.stableDown = true; pressedEdge = true; }
  if (touch.stableDown && stableUp) { touch.stableDown = false; releasedEdge = true; }

  if (touch.stableDown || pressedEdge) {
    int ty = touch.y;
    int tx = touch.x;
    touch.lastBtn = -1;
const int SAFE = 8;                 // évite les zones chiantes tout au bord
if (tx < SAFE) tx = SAFE;
if (tx > SW - 1 - SAFE) tx = SW - 1 - SAFE;
// --- Détection des 2 boutons du haut (Manger / Boire) ---
const int TOP_BTN_Y = 77;
const int TOP_BTN_W = 120;
const int TOP_BTN_H = 18;
const int TOP_BTN_PAD = 8;
const int TOP_BTN_TOUCH_PAD = TOP_BTN_H; // zone tactile agrandie

bool canTopBtns = (phase == PHASE_ALIVE) && (appMode == MODE_PET) && (!task.active) && (state != ST_SLEEP) && (!activityVisible);

#if USE_TOP_QUICK_BUTTONS
if (canTopBtns && ty >= (TOP_BTN_Y - TOP_BTN_TOUCH_PAD) && ty < (TOP_BTN_Y + TOP_BTN_H + TOP_BTN_TOUCH_PAD)) {
  int xL0 = TOP_BTN_PAD;
  int xL1 = xL0 + TOP_BTN_W;
  int xR1 = SW - TOP_BTN_PAD;
  int xR0 = xR1 - TOP_BTN_W;

  if (tx >= xL0 && tx < xL1) { touch.lastBtn = -10; }      // Manger
  else if (tx >= xR0 && tx < xR1) { touch.lastBtn = -11; } // Boire
  else { touch.lastBtn = -1; }
}
#endif

  if (touch.lastBtn != -10 && touch.lastBtn != -11) {
  if (interactionMenuOpen) {
    const int menuCount = (int)interactionMenuCount();
    const int itemGap = 4;
    const int itemH = 20;
    int menuX = 0, menuY = 0, menuW = 0, menuH = 0;
    calcInteractionMenuRect(menuX, menuY, menuW, menuH);

    if (tx >= menuX && tx < (menuX + menuW) && ty >= menuY && ty < (menuY + menuH)) {
      int relY = ty - (menuY + 4);
      int slot = relY / (itemH + itemGap);
      if (slot >= 0 && slot < menuCount) {
        interactionMenuSel = (uint8_t)slot;
        touch.lastBtn = (int8_t)(-30 - slot);
      }
    }
  }

  if (touch.lastBtn < -29) {
    if (uiSel != 0) uiSel = 0;
    uiSpriteDirty = true;
    uiForceBands = true;
  } else {
  const int TOUCH_PAD_Y = UI_BOT_H; // zone tactile agrandie (haut+bas)
  if (ty >= (SH - UI_BOT_H - TOUCH_PAD_Y) && ty < (SH + TOUCH_PAD_Y)) {
    uint8_t nbtn = uiButtonCount();
    int btnY0 = SH - UI_BOT_H + uiBottomY() - 4;
    int btnY1 = btnY0 + uiBottomButtonH() + 8;

    if (uiShowSceneArrows() && ty >= btnY0 && ty < btnY1) {
      int leftX0 = 0;
      int leftX1 = uiSceneArrowLeftX() + uiSceneArrowW() + 6;
      int rightX0 = uiSceneArrowRightX() - 6;
      int rightX1 = SW;
      if (tx >= leftX0 && tx < leftX1) {
        touch.lastBtn = -20;
      } else if (tx >= rightX0 && tx < rightX1) {
        touch.lastBtn = -21;
      }
    }

if (touch.lastBtn != -20 && touch.lastBtn != -21) {

int GAP = 0;
int bw = 0;
int bh = 0;
int yy = 0;
int startX = 0;
calcBottomActionLayout(nbtn, startX, bw, bh, yy, GAP);

    int best = 0;
    int bestDist = 999999;
    for (int i = 0; i < (int)nbtn; i++) {
      int xx = startX + i * (bw + GAP);
      int cx = xx + bw / 2;
      int d = abs(tx - cx);
      if (d < bestDist) { bestDist = d; best = i; }
    }

    int idx = best;

    if (uiSel != (uint8_t)idx) {
      if (interactionMenuOpen && idx != 0) closeInteractionMenu();
      uiSel = (uint8_t)idx;
      uiSpriteDirty = true;
      uiForceBands  = true;
    }

    touch.lastBtn = (int8_t)idx;
}
  } else {
    touch.lastBtn = -1;
  }
}
}

if (pressedEdge) {
  touch.pressBtn = touch.lastBtn;
  touch.pressStart = now;
  touch.longPressFired = false;
}

if (touch.stableDown) {
  if (touch.lastBtn != touch.pressBtn) {
    touch.pressBtn = touch.lastBtn;
    touch.pressStart = now;
    touch.longPressFired = false;
  }

  if (!touch.longPressFired && touch.lastBtn >= 0 && uiButtonCount() == uiAliveCount()) {
    UiAction heldAction = uiAliveActionAt((uint8_t)touch.lastBtn);
    if (heldAction == UI_AUDIO && (int32_t)(now - touch.pressStart) >= (int32_t)AUDIO_LONG_PRESS_MS) {
      audioVolumePercent = (audioVolumePercent >= 10) ? 1 : 10;
      char tmp[32];
      snprintf(tmp, sizeof(tmp), "\xe9\x9f\xb3\xe9\x87\x8f %u%%", audioVolumePercent);  // 音量 X%
      setMsg(tmp, now, 1500);
      uiSpriteDirty = true;
      uiForceBands  = true;
      if (saveReady) saveNow(now, "audio_vol");
      touch.longPressFired = true;
    }
  }
}


  }

if (releasedEdge) {
  if (touch.longPressFired) {
    touch.lastBtn = -1;
    touch.pressBtn = -1;
    touch.longPressFired = false;
    return;
  }
  if (touch.lastBtn == -10) { // MANGER
    (void)performUiAction(UI_MANGER, now);
    touch.lastBtn = -1;
    touch.pressBtn = -1;
    uiSpriteDirty = true; uiForceBands = true;
    return;
  }
  if (touch.lastBtn == -11) { // BOIRE
    (void)performUiAction(UI_BOIRE, now);
    touch.lastBtn = -1;
    touch.pressBtn = -1;
    uiSpriteDirty = true; uiForceBands = true;
    return;
  }
  if (touch.lastBtn == -20) {
    switchSceneByOffset(-1, now);
    touch.lastBtn = -1;
    touch.pressBtn = -1;
    return;
  }
  if (touch.lastBtn == -21) {
    switchSceneByOffset(+1, now);
    touch.lastBtn = -1;
    touch.pressBtn = -1;
    return;
  }
  if (touch.lastBtn <= -30) {
    int menuIdx = -30 - touch.lastBtn;
    if (menuIdx >= 0 && menuIdx < (int)interactionMenuCount()) {
      interactionMenuSel = (uint8_t)menuIdx;
      (void)performUiAction(interactionMenuActionAt(interactionMenuSel), now);
    }
    touch.lastBtn = -1;
    touch.pressBtn = -1;
    uiSpriteDirty = true; uiForceBands = true;
    return;
  }
  if (touch.lastBtn == 0 && !interactionMenuOpen && uiButtonCount() == uiAliveCount()) {
    toggleInteractionMenu(now);
    touch.lastBtn = -1;
    touch.pressBtn = -1;
    uiSpriteDirty = true; uiForceBands = true;
    return;
  }

  if (touch.lastBtn >= 0) {
    uint8_t idx = (uint8_t)touch.lastBtn;
    if (idx < uiAliveCount()) {
      uiSel = idx;
      (void)performUiAction(uiAliveActionAt(idx), now);
      uiSpriteDirty = true;
      uiForceBands = true;
    }
    touch.lastBtn = -1;
    touch.pressBtn = -1;
  }
}

}
