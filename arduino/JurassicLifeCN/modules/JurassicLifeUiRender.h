#pragma once

// HUD, modal overlay, and UI band rendering.
// Extracted from JurassicLifeCN.ino during the architecture refactor.

static void rebuildUISprites(uint32_t now) {
  uiTop.fillSprite(KEY);

  char line[96];
  bool showStatusLine = false;
  uint16_t statusColor = criticalBlinkOn(now) ? TFT_RED : TFT_WHITE;
#if 0
if (phase == PHASE_EGG || phase == PHASE_HATCHING) {

  // Fallback note: keep these strings ASCII-safe if a board/font has trouble
  // rendering punctuation or extended glyphs during boot screens.
  const char* l1 = "\xe4\xb8\x80\xe9\xa2\x97\xe9\xbe\x99\xe6\xb3\xa1\xe6\xb3\xa1\xe5\x8d\xb5...";               // 涓€棰楅緳娉℃场鍗?..
  const char* l2 = "\xe5\xae\x83\xe8\xbf\x98\xe5\x8f\xaa\xe6\x98\xaf\xe5\xb9\xbc\xe4\xbd\x93\xe3\x80\x82"; // 瀹冭繕鍙槸骞间綋銆?
  const char* l3 = "\xe5\x8d\x9a\xe5\xa3\xab\xef\xbc\x8c\xe4\xbd\xa0\xe5\xb7\xb2\xe6\x8e\xa5\xe6\x89\x8b"; // 鍗氬＋锛屼綘宸叉帴鎵?
  const char* l4 = "\xe8\xbf\x99\xe4\xbb\xbd\xe7\xbd\x97\xe5\xbe\xb7\xe5\xb2\x9b\xe7\x85\xa7\xe6\x8a\xa4\xe3\x80\x82"; // 杩欎唤缃楀痉宀涚収鎶ゃ€?

  // Positionnement
  int x = 10;
  int y = 6;

  // 鏍囬琛?- 浣跨敤涓枃瀛椾綋
  setCNFont(uiTop);
  uiTop.setTextColor(TFT_BLACK, KEY);
  uiTop.setCursor(x + 1, y + 1); uiTop.print(l1);
  uiTop.setTextColor(TFT_WHITE, KEY);
  uiTop.setCursor(x,     y);     uiTop.print(l1);

  // 鍚庣画琛?
  int y2 = y + 18;
  uiTop.setTextColor(TFT_BLACK, KEY);
  uiTop.setCursor(x + 1, y2 + 1);        uiTop.print(l2);
  uiTop.setCursor(x + 1, y2 + 16 + 1);   uiTop.print(l3);
  uiTop.setCursor(x + 1, y2 + 32 + 1);   uiTop.print(l4);

  uiTop.setTextColor(TFT_WHITE, KEY);
  uiTop.setCursor(x,     y2);            uiTop.print(l2);
  uiTop.setCursor(x,     y2 + 16);       uiTop.print(l3);
  uiTop.setCursor(x,     y2 + 32);       uiTop.print(l4);
  resetFont(uiTop);

  showStatusLine = false;  // IMPORTANT: sinon il r茅-affiche "line" en haut
}
 else if (phase == PHASE_RESTREADY) {
    const char* l1 = "\xe8\xae\xb0\xe5\xbd\x95\xe5\xae\x8c\xe6\x88\x90!";               // 璁板綍瀹屾垚!
    const char* l2 = "\xe5\xa4\x9a\xe4\xba\x8f\xe5\x8d\x9a\xe5\xa3\xab\xe7\x9a\x84\xe7\x85\xa7\xe6\x8a\xa4,"; // 澶氫簭鍗氬＋鐨勭収鎶?
    const char* l3 = "\xe8\xbf\x99\xe5\x8f\xaa\xe9\xbe\x99\xe6\xb3\xa1\xe6\xb3\xa1";     // 杩欏彧榫欐场娉?
    const char* l4 = "\xe5\xb9\xb3\xe7\xa8\xb3\xe5\xba\xa6\xe8\xbf\x87\xe4\xba\x86\xe6\x88\x90\xe9\x95\xbf\xe9\x98\xb6\xe6\xae\xb5\xe3\x80\x82"; // 骞崇ǔ搴﹁繃浜嗘垚闀块樁娈点€?
    const char* l5 = "\xe6\x96\xb0\xe7\x9a\x84\xe6\x94\xb6\xe5\xae\xb9\xe4\xbb\xbb\xe5\x8a\xa1";   // 鏂扮殑鏀跺浠诲姟
    const char* l6 = "\xe5\xb7\xb2\xe7\xbb\x8f\xe5\x9c\xa8\xe7\xbd\x97\xe5\xbe\xb7\xe5\xb2\x9b\xe5\xbe\x85\xe5\x91\xbd\xe3\x80\x82";   // 宸茬粡鍦ㄧ綏寰峰矝寰呭懡銆?

    setCNFont(uiTop);
    uiTop.setTextColor(TFT_BLACK, KEY);
    uiTop.setCursor(11,  9);  uiTop.print(l1);
    uiTop.setCursor(11, 23);  uiTop.print(l2);
    uiTop.setCursor(11, 37);  uiTop.print(l3);
    uiTop.setCursor(11, 51);  uiTop.print(l4);
    uiTop.setCursor(11, 65);  uiTop.print(l5);
    uiTop.setCursor(11, 79);  uiTop.print(l6);

    uiTop.setTextColor(statusColor, KEY);
    uiTop.setCursor(10,  8);  uiTop.print(l1);
    uiTop.setCursor(10, 22);  uiTop.print(l2);
    uiTop.setCursor(10, 36);  uiTop.print(l3);
    uiTop.setCursor(10, 50);  uiTop.print(l4);
    uiTop.setCursor(10, 64);  uiTop.print(l5);
    uiTop.setCursor(10, 78);  uiTop.print(l6);
    resetFont(uiTop);
  } else if (phase == PHASE_TOMB || phase == PHASE_RESTREADY) {

  // 4 lignes (茅vite les accents si ton font les g猫re mal)
  const char* l1 = "\xe5\x8d\x9a\xe5\xa3\xab\xe7\x96\x8f\xe4\xba\x8e\xe5\x80\xbc\xe5\xae\x88\xe3\x80\x82";       // 鍗氬＋鐤忎簬鍊煎畧銆?
  const char* l2 = "\xe8\xbf\x99\xe5\x8f\xaa\xe9\xbe\x99\xe6\xb3\xa1\xe6\xb3\xa1";               // 杩欏彧榫欐场娉?
  const char* l3 = "\xe6\x9c\xac\xe5\xba\x94\xe5\x9c\xa8\xe7\xbd\x97\xe5\xbe\xb7\xe5\xb2\x9b";                         // 鏈簲鍦ㄧ綏寰峰矝
  const char* l4 = "\xe7\xbb\xa7\xe7\xbb\xad\xe9\x95\xbf\xe5\xa4\xa7\xe3\x80\x82"; // 缁х画闀垮ぇ銆?

  // 浣跨敤涓枃瀛椾綋
  setCNFont(uiTop);

  // 闃村奖锛堥粦鑹诧級
  uiTop.setTextColor(TFT_BLACK, KEY);
  uiTop.setCursor(7,  6);  uiTop.print(l1);
  uiTop.setCursor(7,  22); uiTop.print(l2);
  uiTop.setCursor(7,  38); uiTop.print(l3);
  uiTop.setCursor(7,  54); uiTop.print(l4);

  uiTop.setTextColor(TFT_WHITE, KEY);
  uiTop.setCursor(6,  5);  uiTop.print(l1);
  uiTop.setCursor(6,  21); uiTop.print(l2);
  uiTop.setCursor(6,  37); uiTop.print(l3);
  uiTop.setCursor(6,  53); uiTop.print(l4);
  resetFont(uiTop);

  // IMPORTANT: on sort ici sinon le code plus bas r茅-affiche autre chose

  } else {
    snprintf(line, sizeof(line), "\xe6\xa1\xa3\xe9\xbe\x84:%lum %s %s \xe5\xb8\x81:%lu",
             (unsigned long)pet.ageMin,
             stageLabel(pet.stage),
             sceneLabel(currentScene),
             (unsigned long)pet.lungmenCoin);
    showStatusLine = true;
  }
#endif

  if (phase == PHASE_EGG || phase == PHASE_HATCHING) {
    const char* l1 = "\xe4\xb8\x80\xe9\xa2\x97\xe9\xbe\x99\xe6\xb3\xa1\xe6\xb3\xa1\xe5\x8d\xb5...";
    const char* l2 = "\xe5\xae\x83\xe8\xbf\x98\xe5\x8f\xaa\xe6\x98\xaf\xe5\xb9\xbc\xe4\xbd\x93\xe3\x80\x82";
    const char* l3 = "\xe5\x8d\x9a\xe5\xa3\xab\xef\xbc\x8c\xe4\xbd\xa0\xe5\xb7\xb2\xe6\x8e\xa5\xe6\x89\x8b";
    const char* l4 = "\xe8\xbf\x99\xe4\xbb\xbd\xe7\xbd\x97\xe5\xbe\xb7\xe5\xb2\x9b\xe7\x85\xa7\xe6\x8a\xa4\xe3\x80\x82";
    const int x = 10;
    const int y = 6;
    const int y2 = y + 18;

    setCNFont(uiTop);
    uiTop.setTextColor(TFT_BLACK, KEY);
    uiTop.setCursor(x + 1, y + 1);   uiTop.print(l1);
    uiTop.setCursor(x + 1, y2 + 1);  uiTop.print(l2);
    uiTop.setCursor(x + 1, y2 + 17); uiTop.print(l3);
    uiTop.setCursor(x + 1, y2 + 33); uiTop.print(l4);

    uiTop.setTextColor(TFT_WHITE, KEY);
    uiTop.setCursor(x, y);       uiTop.print(l1);
    uiTop.setCursor(x, y2);      uiTop.print(l2);
    uiTop.setCursor(x, y2 + 16); uiTop.print(l3);
    uiTop.setCursor(x, y2 + 32); uiTop.print(l4);
    resetFont(uiTop);
  } else if (phase == PHASE_RESTREADY) {
    const char* l1 = "\xe8\xae\xb0\xe5\xbd\x95\xe5\xae\x8c\xe6\x88\x90!";
    const char* l2 = "\xe5\xa4\x9a\xe4\xba\x8f\xe5\x8d\x9a\xe5\xa3\xab\xe7\x9a\x84\xe7\x85\xa7\xe6\x8a\xa4,";
    const char* l3 = "\xe8\xbf\x99\xe5\x8f\xaa\xe9\xbe\x99\xe6\xb3\xa1\xe6\xb3\xa1";
    const char* l4 = "\xe5\xb9\xb3\xe7\xa8\xb3\xe5\xba\xa6\xe8\xbf\x87\xe4\xba\x86\xe6\x88\x90\xe9\x95\xbf\xe9\x98\xb6\xe6\xae\xb5\xe3\x80\x82";
    const char* l5 = "\xe6\x96\xb0\xe7\x9a\x84\xe6\x94\xb6\xe5\xae\xb9\xe4\xbb\xbb\xe5\x8a\xa1";
    const char* l6 = "\xe5\xb7\xb2\xe7\xbb\x8f\xe5\x9c\xa8\xe7\xbd\x97\xe5\xbe\xb7\xe5\xb2\x9b\xe5\xbe\x85\xe5\x91\xbd\xe3\x80\x82";

    setCNFont(uiTop);
    uiTop.setTextColor(TFT_BLACK, KEY);
    uiTop.setCursor(11,  9); uiTop.print(l1);
    uiTop.setCursor(11, 23); uiTop.print(l2);
    uiTop.setCursor(11, 37); uiTop.print(l3);
    uiTop.setCursor(11, 51); uiTop.print(l4);
    uiTop.setCursor(11, 65); uiTop.print(l5);
    uiTop.setCursor(11, 79); uiTop.print(l6);

    uiTop.setTextColor(statusColor, KEY);
    uiTop.setCursor(10,  8); uiTop.print(l1);
    uiTop.setCursor(10, 22); uiTop.print(l2);
    uiTop.setCursor(10, 36); uiTop.print(l3);
    uiTop.setCursor(10, 50); uiTop.print(l4);
    uiTop.setCursor(10, 64); uiTop.print(l5);
    uiTop.setCursor(10, 78); uiTop.print(l6);
    resetFont(uiTop);
  } else if (phase == PHASE_TOMB) {
    const char* l1 = "\xe5\x8d\x9a\xe5\xa3\xab\xe7\x96\x8f\xe4\xba\x8e\xe5\x80\xbc\xe5\xae\x88\xe3\x80\x82";
    const char* l2 = "\xe8\xbf\x99\xe5\x8f\xaa\xe9\xbe\x99\xe6\xb3\xa1\xe6\xb3\xa1";
    const char* l3 = "\xe6\x9c\xac\xe5\xba\x94\xe5\x9c\xa8\xe7\xbd\x97\xe5\xbe\xb7\xe5\xb2\x9b";
    const char* l4 = "\xe7\xbb\xa7\xe7\xbb\xad\xe9\x95\xbf\xe5\xa4\xa7\xe3\x80\x82";

    setCNFont(uiTop);
    uiTop.setTextColor(TFT_BLACK, KEY);
    uiTop.setCursor(7,  6); uiTop.print(l1);
    uiTop.setCursor(7, 22); uiTop.print(l2);
    uiTop.setCursor(7, 38); uiTop.print(l3);
    uiTop.setCursor(7, 54); uiTop.print(l4);

    uiTop.setTextColor(TFT_WHITE, KEY);
    uiTop.setCursor(6,  5); uiTop.print(l1);
    uiTop.setCursor(6, 21); uiTop.print(l2);
    uiTop.setCursor(6, 37); uiTop.print(l3);
    uiTop.setCursor(6, 53); uiTop.print(l4);
    resetFont(uiTop);
  } else {
    snprintf(line, sizeof(line), "\xe6\xa1\xa3\xe9\xbe\x84:%lum %s %s \xe5\xb8\x81:%lu",
             (unsigned long)pet.ageMin,
             stageLabel(pet.stage),
             sceneLabel(currentScene),
             (unsigned long)pet.lungmenCoin);
    showStatusLine = true;
  }

  if (showStatusLine) {
    uiTop.setTextSize(1);
    uiTextShadow(uiTop, 6, 2, line, statusColor, KEY);
  }

  int nameW = 0;
if (phase == PHASE_ALIVE) {
    setCNFont(uiTop);
    uiTop.setTextDatum(top_right);
    uiTop.setTextSize(1);
    uiTop.setTextColor(TFT_WHITE, KEY);
    uiTop.drawString(petName, SW - 6, 2);
    uiTop.setTextDatum(top_left);
    nameW = uiTop.textWidth(petName);
    resetFont(uiTop);
  }

  if (phase == PHASE_ALIVE) {
    uint32_t need = (pet.stage == AGE_JUNIOR) ? EVOLVE_JUNIOR_TO_ADULT_MIN
                 : (pet.stage == AGE_ADULTE) ? EVOLVE_ADULT_TO_SENIOR_MIN
                 : EVOLVE_SENIOR_TO_REST_MIN;
    float p = (need > 0) ? (100.0f * (float)min(pet.evolveProgressMin, need) / (float)need) : 0;

    int leftW = uiTop.textWidth(line);
    int x0 = 6 + leftW + 6;
    int x1 = (SW - 6) - nameW - 8;
    int w = x1 - x0;
    int h = 6;
    int y = 3;

    if (w >= 40) {
      uiTop.drawRoundRect(x0, y, w, h, 3, 0xFFFF);
      uiTop.fillRoundRect(x0+1, y+1, w-2, h-2, 2, TFT_WHITE);
      int fw = (int)roundf(((w-2) * p) / 100.0f);
      if (fw > 0) uiTop.fillRoundRect(x0+1, y+1, fw, h-2, 2, 0x07E0);
    }
  }

  if (phase == PHASE_ALIVE) {
    int pad = 6;
    int cols = 3;
    int w = (SW - pad*(cols+1)) / cols;
    int h = 12;

    int y1 = 25;
    int y2 = 50;

    int x = pad;
    drawBarRound(uiTop, x, y1, w, h, pet.faim,    "\xe9\xa5\xb1\xe8\x85\xb9", COL_FAIM);    x += w + pad;   // 楗辫吂
    drawBarRound(uiTop, x, y1, w, h, pet.soif,    "\xe6\xb0\xb4\xe5\x88\x86", COL_SOIF);    x += w + pad;   // 姘村垎
    drawBarRound(uiTop, x, y1, w, h, (100.0f - pet.fatigue), "\xe7\x96\xb2\xe5\x8a\xb3", COL_FATIGUE);      // 鐤插姵

    x = pad;
    drawBarRound(uiTop, x, y2, w, h, pet.hygiene, "\xe5\x8d\xab\xe7\x94\x9f", COL_HYGIENE); x += w + pad;   // 鍗敓
    drawBarRound(uiTop, x, y2, w, h, pet.humeur,  "\xe5\xbf\x83\xe6\x83\x85", COL_HUMEUR);  x += w + pad;   // 蹇冩儏
    drawBarRound(uiTop, x, y2, w, h, pet.amour,   "\xe4\xba\xb2\xe5\xaf\x86", COL_AMOUR2);                    // 浜插瘑
  }

  // Zone sous les barres (脿 la place de l'activity bar quand on est libre)
const int TOP_BTN_Y = 77;
const int TOP_BTN_W = 120;
const int TOP_BTN_H = 18;
const int TOP_BTN_PAD = 8;

  bool canTopBtns = (phase == PHASE_ALIVE) && (appMode == MODE_PET) && (!task.active) && (state != ST_SLEEP);

  if (activityVisible) {
    // si une action est en cours -> on affiche la barre
    float p = activityProgress(now);
    drawActivityBar(uiTop, SW/2, TOP_BTN_Y, 140, TOP_BTN_H, p, activityText);
#if USE_TOP_QUICK_BUTTONS
  } else if (canTopBtns) {
    // sinon -> 2 gros boutons Manger/Boire sous les barres
    auto drawTopBtn = [&](int x, const char* lab, uint16_t baseCol, bool disabled) {
      uint16_t fill   = disabled ? 0xC618 : TFT_WHITE;
      uint16_t fg     = TFT_BLACK;
      uint16_t border = baseCol;

      if (!disabled) {
        // style "couleur quand actif" si tu veux, sinon laisse blanc
        // fill = TFT_WHITE;
        // fg = TFT_BLACK;
      }

      uiTop.fillRoundRect(x, TOP_BTN_Y, TOP_BTN_W, TOP_BTN_H, 8, fill);
      uiTop.drawRoundRect(x, TOP_BTN_Y, TOP_BTN_W, TOP_BTN_H, 8, border);

      setCNFont(uiTop);
      uiTop.setTextDatum(middle_center);
      uiTop.setTextColor(fg, fill);
      uiTop.setTextSize(2);
      if (uiTop.textWidth(lab) > (TOP_BTN_W - 6)) uiTop.setTextSize(1);
      uiTop.drawString(lab, x + TOP_BTN_W/2, TOP_BTN_Y + TOP_BTN_H/2);
      uiTop.setTextDatum(top_left);
      resetFont(uiTop);
    };

    bool cdEat   = ((int32_t)(now - cdUntil[(int)UI_MANGER]) < 0);
    bool cdDrink = ((int32_t)(now - cdUntil[(int)UI_BOIRE ]) < 0);

    int xL = TOP_BTN_PAD;
    int xR = SW - TOP_BTN_PAD - TOP_BTN_W;

    drawTopBtn(xL, "\xe5\x96\x82\xe9\xa3\x9f", btnColorForAction(UI_MANGER), cdEat);   // 鍠傞
    drawTopBtn(xR, "\xe5\x96\x9d\xe6\xb0\xb4", btnColorForAction(UI_BOIRE),  cdDrink); // 鍠濇按
#endif
  }


  uiBot.fillSprite(KEY);
  uiBot.setTextDatum(middle_center);

  const uint8_t nbtn = uiButtonCount();
  const int r = 8;
  int GAP = 0;
  int yy = 0;
  int bw = 0;
  int bh = 0;
  int startX = 0;
  calcBottomActionLayout(nbtn, startX, bw, bh, yy, GAP);
// --- Layout barre du bas (centr茅e, largeur fixe) ---
  bool hardBusy = (phase == PHASE_HATCHING || phase == PHASE_TOMB);
  bool canSceneSwitch = canOpenSceneSelect();

  if (uiShowSceneArrows()) {
    auto drawSceneArrow = [&](int x, bool toRight, bool selected) {
      uint16_t baseCol = 0x03FF;
      uint16_t fill = selected ? baseCol : TFT_WHITE;
      uint16_t fg = selected ? TFT_WHITE : TFT_BLACK;
      uint16_t border = canSceneSwitch ? baseCol : 0x7BEF;
      if (!canSceneSwitch) {
        fill = selected ? 0x9CF3 : 0xDEFB;
        fg = TFT_BLACK;
      }

      uiBot.fillRoundRect(x, yy, uiSceneArrowW(), bh, r, fill);
      uiBot.drawRoundRect(x, yy, uiSceneArrowW(), bh, r, border);
      if (selected) {
        uiBot.drawRoundRect(x + 2, yy + 2, uiSceneArrowW() - 4, bh - 4, max(1, r - 2), TFT_WHITE);
      }

      const int cx = x + uiSceneArrowW() / 2;
      const int cy = yy + bh / 2;
      const int triHalfH = max(4, bh / 4);
      const int triHalfW = max(3, uiSceneArrowW() / 3);
      if (toRight) uiBot.fillTriangle(cx - triHalfW, cy - triHalfH, cx - triHalfW, cy + triHalfH, cx + triHalfW, cy, fg);
      else         uiBot.fillTriangle(cx + triHalfW, cy - triHalfH, cx + triHalfW, cy + triHalfH, cx - triHalfW, cy, fg);
    };

    drawSceneArrow(uiSceneArrowLeftX(), false, uiSel == 0);
    drawSceneArrow(uiSceneArrowRightX(), true, uiSel == (uint8_t)(nbtn - 1));
  }

  for (int i = 0; i < (int)nbtn; i++) {
    int xx = startX + i*(bw + GAP);
    bool sel = (i == (int)uiSel);

    uint16_t fill = TFT_WHITE;
    uint16_t fg   = TFT_BLACK;
    uint16_t border = 0x7BEF;

    const char* lab = "?";
    uint16_t baseCol = 0x7BEF;

    bool disabledLook = false;

    if (nbtn == 1) {
      lab = uiSingleLabel();
      baseCol = uiSingleColor();
      fill = sel ? baseCol : TFT_WHITE;
      fg   = sel ? TFT_WHITE : TFT_BLACK;
      border = baseCol;
      if (hardBusy) disabledLook = true;
    } else {
      UiAction a = uiAliveActionAt((uint8_t)i);
      baseCol = btnColorForAction(a);
      lab = (a == UI_AUDIO) ? "" : uiDisplayLabel(a);

      bool onCooldown = ((int32_t)(now - cdUntil[(int)a]) < 0);
      bool busy = task.active || (state == ST_SLEEP) || (appMode != MODE_PET);

      if ((busy || onCooldown) && a != UI_AUDIO) disabledLook = true;

      if (!disabledLook) {
        fill = sel ? baseCol : TFT_WHITE;
        fg   = sel ? TFT_WHITE : TFT_BLACK;
        border = baseCol;
      } else {
        fill = 0xC618;
        fg   = TFT_BLACK;
        border = baseCol;
      }
    }

    if (hardBusy) { fill = 0xC618; fg = TFT_BLACK; border = 0x7BEF; }

    uiBot.fillRoundRect(xx, yy, bw, bh, r, fill);
    uiBot.drawRoundRect(xx, yy, bw, bh, r, border);

    if (sel) {
      uiBot.drawRoundRect(xx + 2, yy + 2, bw - 4, bh - 4, max(1, r - 2), TFT_WHITE);
    }

    if (lab[0] != '\0') {
      setCNFont(uiBot);
      uiBot.setTextColor(fg, fill);
      uiBot.setTextSize(2);
      if (uiBot.textWidth(lab) > (bw - 6)) uiBot.setTextSize(1);
      uiBot.drawString(lab, xx + bw/2, yy + bh/2);
      resetFont(uiBot);
    } else {
      drawAudioIcon(uiBot, xx, yy, bw, bh, fg, audioMode);
    }
  }

}

static void drawModalTextBand(int bandY, int x, int y, const char* text, uint16_t fg, uint16_t bg) {
  if (!text || text[0] == 0) return;
  if (y < bandY - 18 || y > bandY + BAND_H) return;
  setCNFont(band);
  band.setTextColor(fg, bg);
  band.setTextDatum(top_left);
  band.drawString(text, x, y - bandY);
  resetFont(band);
}

static void drawModalOverlayBand(int bandY, int bh) {
  if (!isModalMode()) return;

  band.fillRect(0, 0, SW, bh, 0x2104);

  const int boxX = 12;
  const int boxY = 18;
  const int boxW = SW - 24;
  const int boxH = SH - 36;
  const int localBoxY = boxY - bandY;

  band.fillRect(boxX, localBoxY, boxW, boxH, 0xFFFF);
  band.drawRect(boxX, localBoxY, boxW, boxH, TFT_BLACK);
  band.drawFastHLine(boxX, localBoxY + 30, boxW, 0x7BEF);

  if (appMode == MODE_MODAL_REPORT) {
    drawModalTextBand(bandY, boxX + 12, boxY + 8, "\xe5\x8d\x9a\xe5\xa3\xab\xe7\xa6\xbb\xe5\xb2\x97\xe6\x8a\xa5\xe5\x91\x8a", TFT_BLACK, 0xFFFF);
    for (uint8_t i = 0; i < offlineReportModal.lineCount; ++i) {
      drawModalTextBand(bandY, boxX + 12, boxY + 44 + i * 24, offlineReportModal.lines[i], TFT_BLACK, 0xFFFF);
    }
    drawModalTextBand(bandY, boxX + 12, boxY + boxH - 34, "\xe4\xb8\xad\xe9\x97\xb4OK\xe5\x85\xb3\xe9\x97\xad", 0x39C7, 0xFFFF);
  } else if (appMode == MODE_MODAL_EVENT) {
    const DailyEventNotice* notice = currentDailyEventNotice();
    char line1[56];
    char line2[56];
    char line3[56];
    if (notice) {
      drawModalTextBand(bandY, boxX + 12, boxY + 8, notice->offline ? "\xe7\xa6\xbb\xe5\xb2\x97\xe4\xba\x8b\xe4\xbb\xb6\xe5\x9b\x9e\xe6\x8a\xa5" : "\xe7\xbd\x97\xe5\xbe\xb7\xe5\xb2\x9b\xe5\xbd\x93\xe6\x97\xa5\xe4\xba\x8b\xe4\xbb\xb6", TFT_BLACK, 0xFFFF);
      drawModalTextBand(bandY, boxX + 12, boxY + 44, notice->eventName, TFT_BLACK, 0xFFFF);
      drawModalTextBand(bandY, boxX + 12, boxY + 68, notice->summary, TFT_BLACK, 0xFFFF);
      snprintf(line1, sizeof(line1), "\xe9\xa5\xb1\xe8\x85\xb9%+d \xe6\xb0\xb4\xe5\x88\x86%+d \xe7\x96\xb2\xe5\x8a\xb3%+d", notice->deltaFaim, notice->deltaSoif, notice->deltaFatigue);
      snprintf(line2, sizeof(line2), "\xe5\x8d\xab\xe7\x94\x9f%+d \xe5\xbf\x83\xe6\x83\x85%+d \xe4\xba\xb2\xe5\xaf\x86%+d", notice->deltaHygiene, notice->deltaHumeur, notice->deltaAmour);
      snprintf(line3, sizeof(line3), "\xe5\x81\xa5\xe5\xba\xb7%+d", notice->deltaSante);
      drawModalTextBand(bandY, boxX + 12, boxY + 98, line1, TFT_BLACK, 0xFFFF);
      drawModalTextBand(bandY, boxX + 12, boxY + 122, line2, TFT_BLACK, 0xFFFF);
      drawModalTextBand(bandY, boxX + 12, boxY + 146, line3, TFT_BLACK, 0xFFFF);

      if (phase == PHASE_ALIVE) {
        uint8_t animId = autoBehaviorAnimIdForStage(pet.stage, notice->art);
        uint8_t cnt = triAnimCount(pet.stage, animId);
        if (cnt == 0) cnt = 1;
        int dw = triW(pet.stage);
        int dh = triH(pet.stage);
        int px = boxX + boxW - dw - 8;
        int py = boxY + 42 - bandY;
        band.fillRect(px - 4, py - 4, dw + 8, dh + 8, 0xE71C);
        band.drawRect(px - 4, py - 4, dw + 8, dh + 8, 0xC618);
        drawTriceratopsFrameOnBand(pet.stage, animId, animIdx % cnt, nullptr, px, py, false, 0);
      }
    }
    drawModalTextBand(bandY, boxX + 12, boxY + boxH - 34, "\xe4\xb8\xad\xe9\x97\xb4OK\xe7\xbb\xa7\xe7\xbb\xad", 0x39C7, 0xFFFF);
  } else if (appMode == MODE_RTC_PROMPT) {
    drawModalTextBand(bandY, boxX + 12, boxY + 8, "RTC \xe6\x9c\xaa\xe6\xa0\xa1\xe6\x97\xb6", TFT_BLACK, 0xFFFF);
    drawModalTextBand(bandY, boxX + 12, boxY + 46, "\xe6\xa3\x80\xe6\xb5\x8b\xe5\x88\xb0RTC\xe4\xbd\x86\xe6\x97\xb6\xe9\x97\xb4\xe6\x97\xa0\xe6\x95\x88", TFT_BLACK, 0xFFFF);
    drawModalTextBand(bandY, boxX + 12, boxY + 70, "\xe5\xbb\xba\xe8\xae\xae\xe5\x85\x88\xe6\xa0\xa1\xe6\x97\xb6\xe5\x86\x8d\xe5\x90\xaf\xe7\x94\xa8\xe7\xa6\xbb\xe7\xba\xbf\xe7\xbb\x93\xe7\xae\x97", TFT_BLACK, 0xFFFF);

    uint16_t leftBg = (rtcPromptModal.sel == 0) ? 0xFD20 : 0xDEFB;
    uint16_t rightBg = (rtcPromptModal.sel == 1) ? 0x07E0 : 0xDEFB;
    band.fillRoundRect(boxX + 14, boxY + 106 - bandY, 78, 28, 6, leftBg);
    band.drawRoundRect(boxX + 14, boxY + 106 - bandY, 78, 28, 6, TFT_BLACK);
    band.fillRoundRect(boxX + boxW - 92, boxY + 106 - bandY, 78, 28, 6, rightBg);
    band.drawRoundRect(boxX + boxW - 92, boxY + 106 - bandY, 78, 28, 6, TFT_BLACK);
    drawModalTextBand(bandY, boxX + 34, boxY + 113, "\xe8\xb7\xb3\xe8\xbf\x87", TFT_BLACK, leftBg);
    drawModalTextBand(bandY, boxX + boxW - 72, boxY + 113, "\xe6\xa0\xa1\xe6\x97\xb6", TFT_BLACK, rightBg);
    drawModalTextBand(bandY, boxX + 12, boxY + boxH - 34, "\xe5\xb7\xa6\xe5\x8f\xb3\xe9\x80\x89\xe6\x8b\xa9  \xe4\xb8\xad\xe9\x97\xb4OK\xe7\xa1\xae\xe8\xae\xa4", 0x39C7, 0xFFFF);
  } else if (appMode == MODE_RTC_SET) {
    char line[48];
    drawModalTextBand(bandY, boxX + 12, boxY + 8, "RTC \xe6\xa0\xa1\xe6\x97\xb6", TFT_BLACK, 0xFFFF);
    static const char* labels[5] = {
      "\xe5\xb9\xb4", "\xe6\x9c\x88", "\xe6\x97\xa5", "\xe6\x97\xb6", "\xe5\x88\x86"
    };
    int values[5] = { rtcDraft.year, rtcDraft.month, rtcDraft.day, rtcDraft.hour, rtcDraft.minute };
    for (int i = 0; i < 5; ++i) {
      int rowY = boxY + 44 + i * 28;
      uint16_t rowBg = (rtcDraft.field == i) ? 0xC7F9 : 0xFFFF;
      band.fillRect(boxX + 10, rowY - bandY, boxW - 20, 22, rowBg);
      snprintf(line, sizeof(line), "%s: %02d", labels[i], values[i]);
      if (i == 0) snprintf(line, sizeof(line), "%s: %04d", labels[i], values[i]);
      drawModalTextBand(bandY, boxX + 18, rowY + 2, line, TFT_BLACK, rowBg);
    }
    drawModalTextBand(bandY, boxX + 12, boxY + boxH - 34, "\xe5\xb7\xa6\xe5\x87\x8f \xe5\x8f\xb3\xe5\x8a\xa0 \xe4\xb8\xad\xe9\x97\xb4OK\xe4\xb8\x8b\xe4\xb8\x80\xe9\xa1\xb9/\xe4\xbf\x9d\xe5\xad\x98", 0x39C7, 0xFFFF);
  } else if (appMode == MODE_SCENE_SELECT) {
    drawModalTextBand(bandY, boxX + 12, boxY + 8, "\xe5\x9c\xba\xe6\x99\xaf\xe5\x88\x87\xe6\x8d\xa2", TFT_BLACK, 0xFFFF);
    drawModalTextBand(bandY, boxX + 12, boxY + 40, "\xe9\x80\x89\xe6\x8b\xa9\xe9\xbe\x99\xe6\xb3\xa1\xe6\xb3\xa1\xe5\xbd\x93\xe5\x89\x8d\xe6\x89\x80\xe5\x9c\xa8\xe5\x9c\xba\xe6\x99\xaf", TFT_BLACK, 0xFFFF);
    for (int i = 0; i < (int)SCENE_COUNT; ++i) {
      int rowY = boxY + 72 + i * 28;
      uint16_t rowBg = (sceneModal.sel == (SceneId)i) ? 0xC7F9 : 0xFFFF;
      band.fillRect(boxX + 10, rowY - bandY, boxW - 20, 22, rowBg);
      drawModalTextBand(bandY, boxX + 18, rowY + 2, sceneLabel((SceneId)i), TFT_BLACK, rowBg);
    }
    drawModalTextBand(bandY, boxX + 12, boxY + boxH - 34, "\xe5\xb7\xa6\xe5\x8f\xb3\xe5\x88\x87\xe6\x8d\xa2 \xe4\xb8\xad\xe9\x97\xb4OK\xe7\xa1\xae\xe8\xae\xa4", 0x39C7, 0xFFFF);
  }
  const int btnY = SH - 48;
  const int btnH = 34;
  const int btnW = (SW - 36) / 3;
  for (int i = 0; i < 3; ++i) {
    int bx = 9 + i * (btnW + 9);
    uint16_t fill = (i == 1) ? 0xC7F9 : 0xDEFB;
    band.fillRoundRect(bx, btnY - bandY, btnW, btnH, 6, fill);
    band.drawRoundRect(bx, btnY - bandY, btnW, btnH, 6, TFT_BLACK);
  }
  drawModalTextBand(bandY, 24, btnY + 8, "\xe5\xb7\xa6", TFT_BLACK, 0xDEFB);      // 宸?
  drawModalTextBand(bandY, SW / 2 - 12, btnY + 8, "OK", TFT_BLACK, 0xC7F9);
  drawModalTextBand(bandY, SW - 34, btnY + 8, "\xe5\x8f\xb3", TFT_BLACK, 0xDEFB);  // 鍙?
}

// ================== Overlay UI into band ==================
static inline void overlayKeyedSpriteIntoBand(int dstYInBand, const uint16_t* src, int srcW, int srcH, int srcY0, int copyH) {
  uint16_t* dst = (uint16_t*)band.getBuffer();
  for (int j = 0; j < copyH; j++) {
    int sy = srcY0 + j;
    int dy = dstYInBand + j;
    if (sy < 0 || sy >= srcH) continue;
    if (dy < 0 || dy >= band.height()) continue;

    const uint16_t* srow = src + sy * srcW;
    uint16_t* drow = dst + dy * SW;

    for (int x = 0; x < SW; x++) {
      uint16_t c = srow[x];
      if (c == KEY || c == KEY_SWAP) continue;
      if (swap16(c) == KEY) continue;
      drow[x] = c;
    }
  }
}
static void overlayUIIntoBand(int bandY, int bh) {
  int top0 = 0, top1 = UI_TOP_H;
  int a0 = max(bandY, top0);
  int a1 = min(bandY + bh, top1);
  if (a1 > a0) {
    int h = a1 - a0;
    int srcY = a0 - top0;
    int dstY = a0 - bandY;
    overlayKeyedSpriteIntoBand(dstY, (uint16_t*)uiTop.getBuffer(), SW, UI_TOP_H, srcY, h);
  }

  int bot0 = SH - UI_BOT_H, bot1 = SH;
  int b0 = max(bandY, bot0);
  int b1 = min(bandY + bh, bot1);
  if (b1 > b0) {
    int h = b1 - b0;
    int srcY = b0 - bot0;
    int dstY = b0 - bandY;
    overlayKeyedSpriteIntoBand(dstY, (uint16_t*)uiBot.getBuffer(), SW, UI_BOT_H, srcY, h);
  }

  if (interactionMenuOpen && phase == PHASE_ALIVE && appMode == MODE_PET) {
    int menuX = 0, menuY = 0, menuW = 0, menuH = 0;
    calcInteractionMenuRect(menuX, menuY, menuW, menuH);
    const int itemGap = 4;
    const int itemH = 20;
    const int menuCount = (int)interactionMenuCount();
    const int a0 = max(bandY, menuY);
    const int a1 = min(bandY + bh, menuY + menuH);
    if (a1 > a0) {
      band.fillRoundRect(menuX, menuY - bandY, menuW, menuH, 8, 0xFFFF);
      band.drawRoundRect(menuX, menuY - bandY, menuW, menuH, 8, btnColorForAction(UI_INTERACT));
      for (int i = 0; i < menuCount; ++i) {
        UiAction menuAction = interactionMenuActionAt((uint8_t)i);
        const int itemY = menuY + 4 + i * (itemH + itemGap);
        const bool selected = (i == (int)interactionMenuSel);
        const bool onCooldown = ((int32_t)(millis() - cdUntil[(int)menuAction]) < 0);
        uint16_t fill = selected ? btnColorForAction(menuAction) : 0xFFFF;
        uint16_t fg = selected ? 0xFFFF : 0x0000;
        uint16_t border = btnColorForAction(menuAction);
        if (onCooldown) {
          fill = 0xDEFB;
          fg = 0x0000;
        }
        band.fillRoundRect(menuX + 4, itemY - bandY, menuW - 8, itemH, 6, fill);
        band.drawRoundRect(menuX + 4, itemY - bandY, menuW - 8, itemH, 6, border);
        drawModalTextBand(bandY, menuX + 12, itemY + 4, uiDisplayLabel(menuAction), fg, fill);
      }
    }
  }

  drawModalOverlayBand(bandY, bh);
}

// ================== DECOR ==================
static void drawMountainImagesBand(float camX, int bandY) {
  if (!currentSceneConfig().showMountains) return;
  int w = 0;
  int h = 0;
  if (!runtimeSingleDimensions(BeansAssets::AssetId::SceneCommonPropMountain, w, h)) return;
  const int yOnGround = GROUND_Y - h;

  float px = camX * 0.25f;
  const int spacing = 180;

  int first = (int)floor((px - SW) / spacing) - 2;
  int last  = (int)floor((px + 2 * SW) / spacing) + 2;

  for (int i = first; i <= last; i++) {
    uint32_t hh = hash32(i * 911);
    int jitter = (int)(hh % 50) - 25;
    int x = i * spacing - (int)px + jitter;
    int yLocal = yOnGround - bandY;
    if (yLocal >= band.height() || yLocal + h <= 0) continue;
      drawRuntimeSingleAssetOnBand(BeansAssets::AssetId::SceneCommonPropMountain, x, yLocal);
  }
}

static void drawSceneCloudsBand(float camX, int bandY) {
  if (!currentSceneConfig().showClouds) return;
  int w = 0;
  int h = 0;
  if (!runtimeSingleDimensions(BeansAssets::AssetId::SceneCommonPropCloud, w, h)) return;
  float px = camX * 0.18f;
  const int spacing = 96;
  int first = (int)floor((px - SW) / spacing) - 2;
  int last  = (int)floor((px + 2 * SW) / spacing) + 2;
  for (int i = first; i <= last; ++i) {
    int x = i * spacing - (int)px + (int)(hash32(i * 471) % 20) - 10;
    int yOnSky = 10 + (int)(hash32(i * 613) % 28);
    int yLocal = yOnSky - bandY;
    if (yLocal >= band.height() || yLocal + h <= 0) continue;
    drawRuntimeSingleAssetOnBand(BeansAssets::AssetId::SceneCommonPropCloud, x, yLocal, false, 0);
  }
}
