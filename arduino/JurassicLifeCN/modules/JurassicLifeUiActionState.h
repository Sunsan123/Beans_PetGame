#pragma once

// UI action metadata, labels, and interaction menu state.
// Extracted from JurassicLifeCN.ino during the architecture refactor.

static inline uint16_t btnColorForAction(UiAction a) {
  switch (a) {
    case UI_REPOS:        return COL_FATIGUE;
    case UI_MANGER:       return COL_FAIM;
    case UI_BOIRE:        return COL_SOIF;
    case UI_LAVER:        return COL_HYGIENE;
    case UI_JOUER:        return COL_HUMEUR;
    case UI_CACA:         return COL_QINGLI;
    case UI_CALIN:        return COL_AMOUR2;
    case UI_INTERACT:     return 0x2C9F;
    case UI_DRESS_MODE:   return 0xA51F;
    case UI_DRESS_SHOP:   return 0xFC60;
    case UI_AUDIO:        return 0x7BEF;
    default:              return TFT_WHITE;
  }
}

static inline const char* btnLabel(UiAction a) {
  switch (a) {
    case UI_REPOS:        return "\xe4\xbc\x91\xe6\x81\xaf";
    case UI_MANGER:       return "\xe5\x96\x82\xe9\xa3\x9f";
    case UI_BOIRE:        return "\xe5\x96\x9d\xe6\xb0\xb4";
    case UI_LAVER:        return "\xe6\xb4\x97\xe6\xbe\xa1";
    case UI_JOUER:        return "\xe7\x8e\xa9\xe8\x80\x8d";
    case UI_CACA:         return "\xe6\xb8\x85\xe7\x90\x86";
    case UI_CALIN:        return "\xe4\xba\x92\xe5\x8a\xa8";
    case UI_AUDIO:        return "\xe5\xa3\xb0\xe9\x9f\xb3";
    default:              return "?";
  }
}

static inline const char* stageLabel(AgeStage s) {
  switch (s) {
    case AGE_JUNIOR:      return "\xe5\xb9\xbc\xe4\xbd\x93";
    case AGE_ADULTE:      return "\xe6\x88\x90\xe4\xbd\x93";
    case AGE_SENIOR:      return "\xe7\xa8\xb3\xe5\xae\x9a\xe4\xbd\x93";
    default:              return "?";
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

static inline bool isDressModeActive() {
  return appMode == MODE_DRESS && phase == PHASE_ALIVE;
}

static inline bool isDressThemeBrowseActive() {
  return isDressModeActive() && dressThemePanelOpen && dressTab == DRESS_TAB_THEME;
}

static inline bool isDressThemeListViewActive() {
  return isDressThemeBrowseActive() && dressThemePanelView == DRESS_THEME_VIEW_THEMES;
}

static inline bool isDressThemeComponentViewActive() {
  return isDressThemeBrowseActive() && dressThemePanelView == DRESS_THEME_VIEW_COMPONENTS;
}

static inline uint8_t dressButtonCount() {
  return 4;
}

static inline uint8_t dressTabButtonIndex(DressTab tab) {
  return 1 + (uint8_t)tab;
}

static inline const char* dressButtonLabel(uint8_t idx) {
  switch (idx) {
    case 0: return "\xe8\xbf\x94\xe5\x9b\x9e";
    case 1: return "\xe4\xb8\xbb\xe9\xa2\x98";
    case 2: return "\xe5\x8d\x95\xe4\xbb\xb6";
    case 3: return "\xe9\xa2\x84\xe8\xae\xbe";
    default: return "?";
  }
}

static inline uint16_t dressButtonColor(uint8_t idx) {
  switch (idx) {
    case 0: return 0xE2C8;
    case 1: return 0x2C9F;
    case 2: return 0x347F;
    case 3: return 0x7BE0;
    default: return 0x7BEF;
  }
}

static inline const char* dressTabLabel(DressTab tab) {
  switch (tab) {
    case DRESS_TAB_THEME:  return "\xe4\xb8\xbb\xe9\xa2\x98";
    case DRESS_TAB_SINGLE: return "\xe5\x8d\x95\xe4\xbb\xb6";
    case DRESS_TAB_PRESET: return "\xe9\xa2\x84\xe8\xae\xbe";
    default: return "?";
  }
}

static inline const char* dressTabTitle(DressTab tab) {
  switch (tab) {
    case DRESS_TAB_THEME:  return "\xe4\xb8\xbb\xe9\xa2\x98\xe6\x96\xb9\xe6\xa1\x88";
    case DRESS_TAB_SINGLE: return "\xe5\x8d\x95\xe4\xbb\xb6\xe6\x90\xad\xe9\x85\x8d";
    case DRESS_TAB_PRESET: return "\xe9\xa2\x84\xe8\xae\xbe\xe9\x80\xa0\xe5\x9e\x8b";
    default: return "?";
  }
}

static inline const char* dressTabHint(DressTab tab) {
  switch (tab) {
    case DRESS_TAB_THEME:  return "\xe5\x88\x87\xe6\x8d\xa2\xe6\x95\xb4\xe4\xbd\x93\xe4\xb8\xbb\xe9\xa2\x98\xe9\xa3\x8e\xe6\xa0\xbc";
    case DRESS_TAB_SINGLE: return "\xe6\x8c\x89\xe9\x83\xa8\xe4\xbd\x8d\xe6\x9b\xbf\xe6\x8d\xa2\xe5\x8d\x95\xe4\xbb\xb6\xe8\xa3\x85\xe6\x89\xae";
    case DRESS_TAB_PRESET: return "\xe4\xbf\x9d\xe5\xad\x98\xe5\xb9\xb6\xe5\xa5\x97\xe7\x94\xa8\xe6\x95\xb4\xe5\xa5\x97\xe6\x90\xad\xe9\x85\x8d";
    default: return "";
  }
}

static inline const char* dressTabStatusLine(DressTab tab) {
  switch (tab) {
    case DRESS_TAB_THEME:  return "\xe4\xb8\xbb\xe9\xa2\x98\xe5\x88\x97\xe8\xa1\xa8\xe6\x95\xb4\xe7\x90\x86\xe4\xb8\xad";
    case DRESS_TAB_SINGLE: return "\xe5\x8d\x95\xe4\xbb\xb6\xe5\x88\x97\xe8\xa1\xa8\xe6\x95\xb4\xe7\x90\x86\xe4\xb8\xad";
    case DRESS_TAB_PRESET: return "\xe9\xa2\x84\xe8\xae\xbe\xe5\x88\x97\xe8\xa1\xa8\xe6\x95\xb4\xe7\x90\x86\xe4\xb8\xad";
    default: return "";
  }
}

static inline uint8_t interactionMenuCount() {
  return (uint8_t)(sizeof(UI_INTERACTION_MENU) / sizeof(UI_INTERACTION_MENU[0]));
}

static inline UiAction interactionMenuActionAt(uint8_t idx) {
  if (idx >= interactionMenuCount()) return UI_REPOS;
  return UI_INTERACTION_MENU[idx];
}

static inline const char* uiDisplayLabel(UiAction a) {
  switch (a) {
    case UI_CALIN:         return "\xe6\x8a\x9a\xe6\x91\xb8";
    case UI_INTERACT:      return "\xe4\xba\x92\xe5\x8a\xa8";
    case UI_DRESS_MODE:    return "\xe8\xa3\x85\xe6\x89\xae\xe6\xa8\xa1\xe5\xbc\x8f";
    case UI_DRESS_SHOP:    return "\xe8\xa3\x85\xe6\x89\xae\xe5\x95\x86\xe5\xba\x97";
    default:               return btnLabel(a);
  }
}

static inline uint8_t uiAliveCount() {
  return (uint8_t)(sizeof(UI_ORDER_ALIVE) / sizeof(UI_ORDER_ALIVE[0]));
}

static inline UiAction uiAliveActionAt(uint8_t idx) {
  if (idx >= uiAliveCount()) return UI_REPOS;
  return UI_ORDER_ALIVE[idx];
}
