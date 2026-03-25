#pragma once

// World band rendering and frame composition.
// Extracted from JurassicLifeCN.ino during the refactor.

static void drawSceneBackdropBand(float camX, int bandY) {
  const SceneConfig& cfg = currentSceneConfig();
  if (!cfg.indoor) {
    if (currentScene == SCENE_DECK) {
      int railY = GROUND_Y - 24 - bandY;
      if (railY >= -8 && railY <= band.height()) {
        band.fillRect(0, railY, SW, 5, cfg.accentB);
        for (int x = -16; x < SW + 24; x += 26) {
          band.fillRect(x - imod((int)camX, 26), railY + 5, 4, 18, cfg.accentA);
        }
      }
    }
    return;
  }

  band.fillRect(0, 0, SW, max(0, GROUND_Y - bandY), cfg.skyColor);
  int panelTop = 16 - bandY;
  for (int x = -20; x < SW + 40; x += 54) {
    int px = x - imod((int)(camX * 0.35f), 54);
    if (panelTop < band.height() && panelTop + 48 > 0) {
      band.drawRect(px, panelTop, 40, 44, cfg.accentB);
      band.drawFastVLine(px + 20, panelTop + 4, 36, cfg.accentB);
    }
  }

  if (currentScene == SCENE_DORM) {
    int bedY = GROUND_Y - 34 - bandY;
    int bedX = SW - 90 - imod((int)(camX * 0.55f), 42);
    if (bedY < band.height() && bedY + 26 > 0) {
      band.fillRoundRect(bedX, bedY, 62, 18, 4, cfg.accentA);
      band.drawRoundRect(bedX, bedY, 62, 18, 4, cfg.accentC);
      band.fillRect(bedX + 6, bedY - 8, 18, 10, 0xFFFF);
    }
  } else if (currentScene == SCENE_ACTIVITY) {
    int matY = GROUND_Y - 18 - bandY;
    int shift = imod((int)(camX * 0.45f), 74);
    for (int x = -10; x < SW + 50; x += 74) {
      band.fillRoundRect(x - shift, matY, 46, 12, 5, ((x / 74) % 2 == 0) ? cfg.accentA : cfg.accentB);
    }
    int toyY = GROUND_Y - 42 - bandY;
    if (toyY < band.height() && toyY + 16 > 0) {
      band.fillRect(34, toyY, 16, 16, cfg.accentC);
      band.fillRect(54, toyY + 6, 12, 10, cfg.accentA);
    }
  }
}

static void drawGroundBand(float camX, int bandY) {
  const SceneConfig& cfg = currentSceneConfig();
  int gy = GROUND_Y - bandY;
  band.fillRect(0, gy, SW, SH - GROUND_Y, cfg.groundColor);
  if (cfg.indoor) {
    for (int y = gy; y < band.height(); y += 10) {
      if (y >= 0) band.drawFastHLine(0, y, SW, cfg.groundLineColor);
    }
    int shift = imod((int)camX, 36);
    for (int x = -40; x < SW + 40; x += 36) {
      int xx = x - shift;
      int lineY = max(0, gy);
      int lineH = max(0, (int)band.height() - gy);
      if (lineH > 0) band.drawFastVLine(xx, lineY, lineH, cfg.groundLineColor);
    }
  } else {
    for (int y = gy; y < band.height(); y += 8) {
      if (y >= 0) band.drawFastHLine(0, y, SW, cfg.groundLineColor);
    }
    int shift = (int)camX;
    for (int x = -40; x < SW + 40; x += 32) {
      int xx = x - imod(shift, 32);
      int yy = (GROUND_Y + 18 + (imod(x + shift, 64) / 8)) - bandY;
      band.fillRect(xx, yy, 10, 3, cfg.accentB);
    }
  }
}
static void drawTreesMixedBand(float camX, int bandY) {
  if (!currentSceneConfig().showTrees) return;
  float px = camX * 0.60f;
  const int spacing = 120;
  int first = (int)floor((px - SW) / spacing) - 3;
  int last  = (int)floor((px + 2 * SW) / spacing) + 3;

  for (int i = first; i <= last; i++) {
    uint32_t hh = hash32(i * 1337);
    if ((hh % 2) != 0) continue;
    int jitter = (int)((hh >> 8) % 50) - 25;
    int x = i * spacing - (int)px + jitter;

    bool useArbre = ((hh % 3) == 0);
    const BeansAssets::AssetId assetId = useArbre
        ? BeansAssets::AssetId::SceneCommonPropTreeBroadleaf
        : BeansAssets::AssetId::SceneCommonPropTreePine;
    int w = 0;
    int h = 0;
    if (!runtimeSingleDimensions(assetId, w, h)) continue;

    int yOnGround = GROUND_Y - h;
    int yLocal = yOnGround - bandY;
    if (yLocal >= band.height() || yLocal + h <= 0) continue;
    drawRuntimeSingleAssetOnBand(assetId, x, yLocal);
  }
}

static void drawSupplyStationBand(float camX, int bandY) {
  const SceneConfig& cfg = currentSceneConfig();
  int w = sceneSupplyPropWidth();
  int h = (currentScene == SCENE_DECK) ? 26 : 22;
  int x = (int)roundf(bushLeftX - camX);
  int yOnGround = GROUND_Y - h + 8;
  int yLocal = yOnGround - bandY;
  if (yLocal >= band.height() || yLocal + h <= 0 || x > SW || x + w < 0) return;

  uint16_t fill = berriesLeftAvailable ? cfg.accentA : 0x9CD3;
  uint16_t border = berriesLeftAvailable ? cfg.accentC : 0x6B4D;
  band.fillRoundRect(x, yLocal, w, h, 4, fill);
  band.drawRoundRect(x, yLocal, w, h, 4, border);
  band.drawFastHLine(x + 4, yLocal + 8, w - 8, border);
  band.drawFastVLine(x + w / 2, yLocal + 4, h - 8, border);
}

static void drawWaterStationBand(float camX, int bandY) {
  const SceneConfig& cfg = currentSceneConfig();
  int w = sceneWaterPropWidth();
  int h = (currentScene == SCENE_DECK) ? 28 : 30;
  int x = (int)roundf(puddleX - camX);
  int yOnGround = GROUND_Y - h + 10;
  int yLocal = yOnGround - bandY;
  if (yLocal >= band.height() || yLocal + h <= 0 || x > SW || x + w < 0) return;

  uint16_t body = puddleVisible ? cfg.accentB : 0xC618;
  band.fillRoundRect(x + 6, yLocal, w - 12, h - 8, 5, 0xFFFF);
  band.drawRoundRect(x + 6, yLocal, w - 12, h - 8, 5, cfg.accentC);
  band.fillRect(x + (w / 2) - 4, yLocal + 8, 8, h - 4, body);
  band.fillCircle(x + (w / 2), yLocal + h - 4, 8, body);
}

static void drawFixedObjectsBand(float camX, int bandY) {
  drawSupplyStationBand(camX, bandY);
  drawWaterStationBand(camX, bandY);

  if (poopVisible) {
    BeansAssets::RuntimeAnimationRef wasteAnim;
    if (!runtimeAnimationInfo("props_gameplay", "prop.gameplay.waste.default", wasteAnim)) return;
    int w = (int)wasteAnim.width;
    int h = (int)wasteAnim.height;
    int x = (int)roundf(poopWorldX - camX);
    int yOnGround = GROUND_Y - h + 18;
    int yLocal = yOnGround - bandY;
    if (!(yLocal >= band.height() || yLocal + h <= 0) && !(x > SW || x + w < 0)) {
      drawRuntimeAnimationFrameOnBand("props_gameplay", "prop.gameplay.waste.default", 2, x, yLocal, w, h);
    }
  }
}

static void drawWorldBand(float camX, int bandY, int bh) {
  band.fillRect(0, 0, SW, BAND_H, currentSceneConfig().skyColor);
  band.setClipRect(0, 0, SW, bh);
  drawSceneCloudsBand(camX, bandY);
  drawMountainImagesBand(camX, bandY);
  drawSceneBackdropBand(camX, bandY);
  drawGroundBand(camX, bandY);
  drawTreesMixedBand(camX, bandY);
  drawFixedObjectsBand(camX, bandY);
}

static void drawAutoBehaviorBubbleOnBand(int bandY, int dinoX, int dinoY) {
  if (phase != PHASE_ALIVE) return;
  if (task.active || appMode != MODE_PET) return;
  if (!autoBehavior.active || autoBehavior.bubble[0] == 0) return;
  if ((int32_t)(millis() - autoBehavior.bubbleUntil) >= 0) return;

  const int dw = triW(pet.stage);
  setCNFont(band);
  band.setTextSize(1);
  int textW = band.textWidth(autoBehavior.bubble);
  resetFont(band);

  int bubbleW = clampi(textW + 18, 58, SW - 8);
  int bubbleH = 22;
  int bubbleX = clampi(dinoX + dw / 2 - bubbleW / 2, 4, SW - bubbleW - 4);
  int bubbleY = max(4, dinoY - bubbleH - 14);
  int localY = bubbleY - bandY;
  if (localY >= band.height() || localY + bubbleH + 10 <= 0) return;

  band.fillRoundRect(bubbleX, localY, bubbleW, bubbleH, 8, 0xFFFF);
  band.drawRoundRect(bubbleX, localY, bubbleW, bubbleH, 8, TFT_BLACK);
  int tailX = clampi(dinoX + dw / 2, bubbleX + 12, bubbleX + bubbleW - 12);
  int tailTipY = localY + bubbleH + 7;
  band.drawLine(tailX - 5, localY + bubbleH - 1, tailX, tailTipY, TFT_BLACK);
  band.drawLine(tailX + 5, localY + bubbleH - 1, tailX, tailTipY, TFT_BLACK);
  band.fillTriangle(tailX - 4, localY + bubbleH - 1, tailX + 4, localY + bubbleH - 1, tailX, tailTipY - 1, 0xFFFF);

  setCNFont(band);
  band.setTextColor(TFT_BLACK, 0xFFFF);
  band.setTextDatum(middle_center);
  band.drawString(autoBehavior.bubble, bubbleX + bubbleW / 2, localY + bubbleH / 2 + 1);
  band.setTextDatum(top_left);
  resetFont(band);
}

// ================== RENDER ==================
static float lastCamX = 0.0f;
static int lastBandMin = -1;
static int lastBandMax = -1;
static uint8_t renderAliveAnimId = 0;
static uint8_t renderAliveFrameIndex = 0;

static inline void pushBandToScreen(int y0, int bh) {
  uint16_t* buf = (uint16_t*)band.getBuffer();
  tft.pushImage(0, y0, SW, bh, buf);
}

static inline void renderOneBand(int y0, int bh, int dinoX, int dinoY, const uint16_t* frame, bool flipX, uint8_t shade) {
  drawWorldBand(camX, y0, bh);

  if (phase == PHASE_ALIVE) {
    int DW = triW(pet.stage);
    int DH = triH(pet.stage);
    if (dinoY < y0 + bh && dinoY + DH > y0) {
      drawTriceratopsFrameOnBand(pet.stage, renderAliveAnimId, renderAliveFrameIndex, frame, dinoX, dinoY - y0, flipX, shade);
    }
    drawAutoBehaviorBubbleOnBand(y0, dinoX, dinoY);
  } else if (phase == PHASE_EGG || phase == PHASE_HATCHING) {
    uint16_t eggFrameIndex = 0;
    if (phase == PHASE_HATCHING) eggFrameIndex = hatchIdx;
    BeansAssets::RuntimeAnimationRef eggAnim;
    if (!runtimeAnimationInfo("character_triceratops", "character.triceratops.egg.hatch", eggAnim)) {
      overlayUIIntoBand(y0, bh);
      pushBandToScreen(y0, bh);
      return;
    }
    int w = (int)eggAnim.width;
    int h = (int)eggAnim.height;
    float t = (float)millis() * 0.008f;
    int bob = (int)roundf(sinf(t) * 2.0f);
    int ex = dinoX;
    int ey = (GROUND_Y - 40) + bob;
    if (ey < y0 + bh && ey + h > y0) {
      drawRuntimeAnimationFrameOnBand("character_triceratops", "character.triceratops.egg.hatch", eggFrameIndex, ex, ey - y0, w, h);
    }
  } else if (phase == PHASE_TOMB || phase == PHASE_RESTREADY) {
    int w = 0;
    int h = 0;
    if (!runtimeSingleDimensions(BeansAssets::AssetId::PropGameplayGraveMarker, w, h)) {
      overlayUIIntoBand(y0, bh);
      pushBandToScreen(y0, bh);
      return;
    }
    int tx = (SW - w) / 2;
    int ty = (GROUND_Y - h + 10);
    if (ty < y0 + bh && ty + h > y0) drawRuntimeSingleAssetOnBand(BeansAssets::AssetId::PropGameplayGraveMarker, tx, ty - y0);
  }

  overlayUIIntoBand(y0, bh);
  pushBandToScreen(y0, bh);
}

static void renderFrameOptimized(int dinoX, int dinoY, const uint16_t* frame, bool flipX, uint8_t shade) {
  bool camMoved = (fabsf(camX - lastCamX) > 0.001f);
  int DH = (phase == PHASE_ALIVE) ? triH(pet.stage)
           : (phase == PHASE_TOMB || phase == PHASE_RESTREADY) ? graveMarkerHeight()
           : 60;

  if (camMoved) {
    for (int y = 0; y < SH; y += BAND_H) {
      int bh = (y + BAND_H <= SH) ? BAND_H : (SH - y);
      renderOneBand(y, bh, dinoX, dinoY, frame, flipX, shade);
    }
    lastBandMin = 0;
    lastBandMax = (SH - 1) / BAND_H;
  } else {
    int ymin = dinoY - 2;
    int ymax = dinoY + DH + 2;
    int bmin = clampi(ymin / BAND_H, 0, (SH - 1) / BAND_H);
    int bmax = clampi(ymax / BAND_H, 0, (SH - 1) / BAND_H);

    if (uiForceBands) {
      int uiTopBandMax = (UI_TOP_H - 1) / BAND_H;
      int botStart = SH - UI_BOT_H;
      int uiBotBandMin = botStart / BAND_H;
      int uiBotBandMax = (SH - 1) / BAND_H;
      bmin = min(bmin, 0);
      bmax = max(bmax, uiTopBandMax);
      bmin = min(bmin, uiBotBandMin);
      bmax = max(bmax, uiBotBandMax);
    }

    if (lastBandMin != -1) bmin = min(bmin, lastBandMin);
    if (lastBandMax != -1) bmax = max(bmax, lastBandMax);

    for (int bi = bmin; bi <= bmax; bi++) {
      int y = bi * BAND_H;
      int bh = (y + BAND_H <= SH) ? BAND_H : (SH - y);
      renderOneBand(y, bh, dinoX, dinoY, frame, flipX, shade);
    }
    lastBandMin = bmin;
    lastBandMax = bmax;
  }

  lastCamX = camX;
}

static void renderGameFrame(uint32_t now) {
  if (uiSpriteDirty) {
    uint8_t nbtn = uiButtonCount();
    if (uiSel >= nbtn) uiSel = 0;
    rebuildUISprites(now);
    uiSpriteDirty = false;
  }

  if (phase == PHASE_ALIVE) {
    uint8_t animId = animIdForState(pet.stage, state);
    if (!task.active && appMode == MODE_PET && autoBehavior.active) {
      AutoBehaviorArt art = (autoBehavior.art == AUTO_ART_AUTO) ? defaultAutoArtForMove(autoBehavior.moveMode) : autoBehavior.art;
      animId = autoBehaviorAnimIdForStage(pet.stage, art);
    }

    bool forceFace = false;
    bool faceRight = false;
    if (task.active && task.kind == TASK_HUG && task.ph == PH_DO) {
      animId = triceratopsLoveAnimId(pet.stage);
    }

    if (task.active && task.ph == PH_DO) {
      if (task.kind == TASK_EAT) { forceFace = true; faceRight = false; }
      if (task.kind == TASK_DRINK) { forceFace = true; faceRight = true; }
    }

    if ((int32_t)(now - nextAnimTick) >= 0) {
      uint8_t cnt = triAnimCount(pet.stage, animId);
      if (cnt == 0) cnt = 1;
      animIdx = (animIdx + 1) % cnt;
      nextAnimTick = now + frameMsForState(state);
    }

    int dinoX = (int)roundf(worldX - camX);
    if (dinoX > DZ_R) camX = worldX - (float)DZ_R;
    else if (dinoX < DZ_L) camX = worldX - (float)DZ_L;
    camX = clampf(camX, 0.0f, worldW - (float)SW);
    dinoX = (int)roundf(worldX - camX);

    int dinoY = (GROUND_Y - TRI_FOOT_Y);
    renderAliveAnimId = animId;
    renderAliveFrameIndex = animIdx;

    bool flipX = flipForMovingRight(movingRight);
    if (forceFace) flipX = faceRight ? true : false;

    uint8_t shade = 0;
    if (pet.hygiene <= 0.0f) shade = 2;
    else if (pet.hygiene < 20.0f) shade = 1;

    tft.startWrite();
    renderFrameOptimized(dinoX, dinoY, nullptr, flipX, shade);
    uiForceBands = false;
    tft.endWrite();
  } else {
    int dinoX = (int)roundf(worldX - camX);
    int dinoY = (GROUND_Y - TRI_FOOT_Y);
    tft.startWrite();
    renderFrameOptimized(dinoX, dinoY, nullptr, false, 0);
    uiForceBands = false;
    tft.endWrite();
  }
}
