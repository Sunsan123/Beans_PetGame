#pragma once

// UI action metadata, labels, and interaction menu state.
// Extracted from JurassicLifeCN.ino during the architecture refactor.

static inline uint16_t btnColorForAction(UiAction a) {
  switch (a) {
    case UI_REPOS:  return COL_FATIGUE;
    case UI_MANGER: return COL_FAIM;
    case UI_BOIRE:  return COL_SOIF;
    case UI_LAVER:  return COL_HYGIENE;
    case UI_JOUER:  return COL_HUMEUR;
    case UI_CACA:   return COL_QINGLI;
    case UI_CALIN:  return COL_AMOUR2;
    case UI_INTERACT: return 0x2C9F;
    case UI_DRESS_MODE: return 0xA51F;
    case UI_DRESS_SHOP: return 0xFC60;
    case UI_AUDIO:  return 0x7BEF;
    default:        return TFT_WHITE;
  }
}
static inline const char* btnLabel(UiAction a) {
  switch (a) {
    case UI_REPOS:  return "\xe4\xbc\x91\xe6\x81\xaf";   // 休息
    case UI_MANGER: return "\xe5\x96\x82\xe9\xa3\x9f";   // 喂食
    case UI_BOIRE:  return "\xe5\x96\x9d\xe6\xb0\xb4";   // 喝水
    case UI_LAVER:  return "\xe6\xb4\x97\xe6\xbe\xa1";   // 洗澡
    case UI_JOUER:  return "\xe7\x8e\xa9\xe8\x80\x8d";   // 玩耍
    case UI_CACA:   return "\xe6\xb8\x85\xe7\x90\x86";   // 清理
    case UI_CALIN:  return "\xe4\xba\x92\xe5\x8a\xa8";   // 互动
    case UI_AUDIO:  return "\xe5\xa3\xb0\xe9\x9f\xb3";   // 声音
    default:        return "?";
  }
}
static inline const char* stageLabel(AgeStage s) {
  switch (s) {
    case AGE_JUNIOR: return "\xe5\xb9\xbc\xe4\xbd\x93";      // 幼体
    case AGE_ADULTE: return "\xe6\x88\x90\xe4\xbd\x93";      // 成体
    case AGE_SENIOR: return "\xe7\xa8\xb3\xe5\xae\x9a\xe4\xbd\x93";  // 稳定体
    default:         return "?";
  }
}

static void drawAudioIcon(LGFX_Sprite& g, int x, int y, int w, int h, uint16_t color, AudioMode mode) {
  int cx = x + w / 2;
  int cy = y + h / 2;
  int noteR = 3;
  int stemH = 10;

  auto drawNote = [&](int nx, int ny) {
    g.fillCircle(nx, ny, noteR, color);
    g.drawFastVLine(nx + noteR, ny - stemH, stemH, color);
    g.drawFastHLine(nx + noteR, ny - stemH, 6, color);
  };

  if (mode == AUDIO_TOTAL) {
    drawNote(cx - 6, cy + 2);
    drawNote(cx + 4, cy - 1);
  } else {
    drawNote(cx - 2, cy + 1);
  }

  if (mode == AUDIO_OFF) {
    g.drawLine(x + 6, y + 6, x + w - 6, y + h - 6, color);
  }
}
// ===== Ordre des boutons (barre du bas) =====
// Change juste l'ordre ici pour déplacer Manger/Boire où tu veux :
#if USE_TOP_QUICK_BUTTONS
static const UiAction UI_ORDER_ALIVE[] = {
  UI_INTERACT, UI_DRESS_MODE, UI_DRESS_SHOP
#if ENABLE_AUDIO
  , UI_AUDIO
#endif
};
#else
static const UiAction UI_ORDER_ALIVE[] = {
  UI_INTERACT, UI_DRESS_MODE, UI_DRESS_SHOP
#if ENABLE_AUDIO
  , UI_AUDIO
#endif
};
#endif

static const UiAction UI_INTERACTION_MENU[] = {
  UI_REPOS, UI_MANGER, UI_LAVER, UI_JOUER, UI_CACA, UI_CALIN
};

static bool interactionMenuOpen = false;
static uint8_t interactionMenuSel = 0;

static inline uint8_t interactionMenuCount() {
  return (uint8_t)(sizeof(UI_INTERACTION_MENU) / sizeof(UI_INTERACTION_MENU[0]));
}

static inline UiAction interactionMenuActionAt(uint8_t idx) {
  if (idx >= interactionMenuCount()) return UI_REPOS;
  return UI_INTERACTION_MENU[idx];
}

static inline const char* uiDisplayLabel(UiAction a) {
  switch (a) {
    case UI_CALIN:       return "\xe6\x8a\x9a\xe6\x91\xb8";   // 抚摸
    case UI_INTERACT:    return "\xe4\xba\x92\xe5\x8a\xa8";   // 互动
    case UI_DRESS_MODE:  return "\xe8\xa3\x85\xe6\x89\xae\xe6\xa8\xa1\xe5\xbc\x8f"; // 装扮模式
    case UI_DRESS_SHOP:  return "\xe8\xa3\x85\xe6\x89\xae\xe5\x95\x86\xe5\xba\x97"; // 装扮商店
    default:             return btnLabel(a);
  }
}

static inline uint8_t uiAliveCount() {
  return (uint8_t)(sizeof(UI_ORDER_ALIVE) / sizeof(UI_ORDER_ALIVE[0]));
}

static inline UiAction uiAliveActionAt(uint8_t idx) {
  if (idx >= uiAliveCount()) return UI_REPOS;
  return UI_ORDER_ALIVE[idx];
}
