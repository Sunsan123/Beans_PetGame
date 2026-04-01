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
static bool lastRenderedDressMode = false;

static inline void pushBandToScreen(int y0, int bh);
static void drawDressThemePreviewBand(int bandY, int bh);

static void drawDressSceneBand(float camX, int bandY, int bh) {
  band.fillRect(0, 0, SW, BAND_H, currentSceneConfig().skyColor);
  band.setClipRect(0, 0, SW, bh);
  drawSceneCloudsBand(camX, bandY);
  drawMountainImagesBand(camX, bandY);
  drawSceneBackdropBand(camX, bandY);
  drawGroundBand(camX, bandY);
  drawTreesMixedBand(camX, bandY);
  if (!dressThemePanelOpen) {
    drawDressThemePreviewBand(bandY, bh);
  }
}

static void drawDressTextOnSprite(LGFX_Sprite& g, int x, int y, const char* text, uint16_t fg, uint16_t bg) {
  if (!text || text[0] == 0) return;
  setCNFont(g);
  g.setTextDatum(top_left);
  g.setTextColor(fg, bg);
  g.drawString(text, x, y);
  resetFont(g);
}

static void drawDressTextOnBand(int bandY, int x, int y, const char* text, uint16_t fg, uint16_t bg) {
  if (!text || text[0] == 0) return;
  if (y < bandY - 18 || y > bandY + BAND_H) return;
  drawDressTextOnSprite(band, x, y - bandY, text, fg, bg);
}

static bool ensureDressThemeGallerySprite(int galleryW, int galleryH) {
  if (dressThemeGallery.getBuffer() && dressThemeGalleryW == galleryW && dressThemeGalleryH == galleryH) {
    return true;
  }
  if (dressThemeGallery.getBuffer()) dressThemeGallery.deleteSprite();
  dressThemeGallery.setPsram(true);
  dressThemeGallery.setColorDepth(16);
  if (!dressThemeGallery.createSprite(galleryW, galleryH)) {
    dressThemeGallery.setPsram(false);
    if (!dressThemeGallery.createSprite(galleryW, galleryH)) {
      dressThemeGalleryW = 0;
      dressThemeGalleryH = 0;
      return false;
    }
  }
  dressThemeGalleryW = galleryW;
  dressThemeGalleryH = galleryH;
  dressThemeGalleryDirty = true;
  return true;
}

static bool ensureDressThemePreviewSprite(int previewW, int previewH) {
  if (dressThemePreview.getBuffer() && dressThemePreviewW == previewW && dressThemePreviewH == previewH) {
    return true;
  }
  if (dressThemePreview.getBuffer()) dressThemePreview.deleteSprite();
  dressThemePreview.setPsram(true);
  dressThemePreview.setColorDepth(16);
  if (!dressThemePreview.createSprite(previewW, previewH)) {
    dressThemePreview.setPsram(false);
    if (!dressThemePreview.createSprite(previewW, previewH)) {
      dressThemePreviewW = 0;
      dressThemePreviewH = 0;
      return false;
    }
  }
  dressThemePreviewW = previewW;
  dressThemePreviewH = previewH;
  dressThemePreviewDirty = true;
  return true;
}

static void overlayDressThemeGallerySpriteIntoBand(int bandY, int bh, int galleryX, int galleryY) {
  if (!dressThemeGallery.getBuffer() || dressThemeGalleryW <= 0 || dressThemeGalleryH <= 0) return;
  int a0 = max(bandY, galleryY);
  int a1 = min(bandY + bh, galleryY + dressThemeGalleryH);
  if (a1 <= a0) return;

  int srcY0 = a0 - galleryY;
  int dstY0 = a0 - bandY;
  int copyH = a1 - a0;
  const uint16_t* src = (const uint16_t*)dressThemeGallery.getBuffer();
  uint16_t* dst = (uint16_t*)band.getBuffer();

  for (int j = 0; j < copyH; ++j) {
    const uint16_t* srow = src + (srcY0 + j) * dressThemeGalleryW;
    uint16_t* drow = dst + (dstY0 + j) * SW + galleryX;
    for (int x = 0; x < dressThemeGalleryW; ++x) {
      uint16_t c = srow[x];
      if (c == KEY || c == KEY_SWAP) continue;
      if (swap16(c) == KEY) continue;
      drow[x] = c;
    }
  }
}

static void overlayDressThemePreviewSpriteIntoBand(int bandY, int bh, int previewX, int previewY) {
  if (!dressThemePreview.getBuffer() || dressThemePreviewW <= 0 || dressThemePreviewH <= 0) return;
  int a0 = max(bandY, previewY);
  int a1 = min(bandY + bh, previewY + dressThemePreviewH);
  if (a1 <= a0) return;

  int srcY0 = a0 - previewY;
  int dstY0 = a0 - bandY;
  int copyH = a1 - a0;
  const uint16_t* src = (const uint16_t*)dressThemePreview.getBuffer();
  uint16_t* dst = (uint16_t*)band.getBuffer();

  for (int j = 0; j < copyH; ++j) {
    const uint16_t* srow = src + (srcY0 + j) * dressThemePreviewW;
    uint16_t* drow = dst + (dstY0 + j) * SW + previewX;
    for (int x = 0; x < dressThemePreviewW; ++x) {
      uint16_t c = srow[x];
      if (c == KEY || c == KEY_SWAP) continue;
      if (swap16(c) == KEY) continue;
      drow[x] = c;
    }
  }
}

static bool readDressPngDimensions(File& file, int& outW, int& outH) {
  outW = 0;
  outH = 0;
  if (!file) return false;
  uint8_t hdr[24];
  if (!file.seek(0)) return false;
  size_t readLen = file.read(hdr, sizeof(hdr));
  file.seek(0);
  if (readLen < sizeof(hdr)) return false;

  static const uint8_t kPngSig[8] = { 137, 80, 78, 71, 13, 10, 26, 10 };
  for (uint8_t i = 0; i < 8; ++i) {
    if (hdr[i] != kPngSig[i]) return false;
  }
  if (hdr[12] != 'I' || hdr[13] != 'H' || hdr[14] != 'D' || hdr[15] != 'R') return false;

  outW = ((int)hdr[16] << 24) | ((int)hdr[17] << 16) | ((int)hdr[18] << 8) | (int)hdr[19];
  outH = ((int)hdr[20] << 24) | ((int)hdr[21] << 16) | ((int)hdr[22] << 8) | (int)hdr[23];
  return outW > 0 && outH > 0;
}

static bool dressThemeIsRgb565PreviewPath(const char* path) {
  if (!path) return false;
  const char* dot = strrchr(path, '.');
  return dot && strcmp(dot, ".rgb565") == 0;
}

static bool readDressRgb565Dimensions(File& file, int& outW, int& outH, size_t& outDataOffset) {
  outW = 0;
  outH = 0;
  outDataOffset = 0;
  if (!file) return false;

  uint8_t hdr[8];
  if (!file.seek(0)) return false;
  size_t readLen = file.read(hdr, sizeof(hdr));
  if (readLen < sizeof(hdr)) return false;
  if (hdr[0] != 'D' || hdr[1] != 'T' || hdr[2] != 'H' || hdr[3] != '1') return false;

  outW = (int)(hdr[4] | (hdr[5] << 8));
  outH = (int)(hdr[6] | (hdr[7] << 8));
  outDataOffset = sizeof(hdr);
  return outW > 0 && outH > 0;
}

static inline float dressPngScaleForBox(int srcW, int srcH, int boxW, int boxH) {
  if (srcW <= 0 || srcH <= 0 || boxW <= 0 || boxH <= 0) return 1.0f;
  float sx = (float)boxW / (float)srcW;
  float sy = (float)boxH / (float)srcH;
  float scale = min(sx, sy);
  if (scale <= 0.0f) scale = 1.0f;
  return scale;
}

static bool ensureDressThemeDecodeSprite(int srcW, int srcH, LGFX_Sprite*& outSprite) {
  static LGFX_Sprite decodeSprite(&tft);
  static int decodeW = 0;
  static int decodeH = 0;
  outSprite = nullptr;
  if (srcW <= 0 || srcH <= 0) return false;

  if (decodeSprite.getBuffer() && decodeW == srcW && decodeH == srcH) {
    outSprite = &decodeSprite;
    return true;
  }

  if (decodeSprite.getBuffer()) decodeSprite.deleteSprite();
  decodeSprite.setPsram(true);
  decodeSprite.setColorDepth(16);
  if (!decodeSprite.createSprite(srcW, srcH)) {
    decodeSprite.setPsram(false);
    if (!decodeSprite.createSprite(srcW, srcH)) {
      decodeW = 0;
      decodeH = 0;
      return false;
    }
  }

  decodeW = srcW;
  decodeH = srcH;
  outSprite = &decodeSprite;
  return true;
}

static void drawScaled565ToSprite(LGFX_Sprite& dst,
                                  const uint16_t* src,
                                  int srcW,
                                  int srcH,
                                  int dstX,
                                  int dstY,
                                  int dstW,
                                  int dstH) {
  if (!src || srcW <= 0 || srcH <= 0 || dstW <= 0 || dstH <= 0) return;
  for (int yy = 0; yy < dstH; ++yy) {
    int sy = ((yy * srcH) / dstH);
    if (sy < 0) sy = 0;
    if (sy >= srcH) sy = srcH - 1;
    for (int xx = 0; xx < dstW; ++xx) {
      int sx = ((xx * srcW) / dstW);
      if (sx < 0) sx = 0;
      if (sx >= srcW) sx = srcW - 1;
      dst.drawPixel(dstX + xx, dstY + yy, src[sy * srcW + sx]);
    }
  }
}

static void drawDress565PixelsToSprite(LGFX_Sprite& dst,
                                       const uint16_t* src,
                                       int srcW,
                                       int srcH,
                                       int dstX,
                                       int dstY) {
  if (!src || srcW <= 0 || srcH <= 0) return;
  for (int yy = 0; yy < srcH; ++yy) {
    for (int xx = 0; xx < srcW; ++xx) {
      dst.drawPixel(dstX + xx, dstY + yy, src[(size_t)yy * (size_t)srcW + (size_t)xx]);
    }
  }
}

static bool drawDressRaw565ThumbToSprite(LGFX_Sprite& g,
                                         File& file,
                                         size_t dataOffset,
                                         int srcW,
                                         int srcH,
                                         int drawX,
                                         int drawY,
                                         int drawW,
                                         int drawH) {
  if (!file || srcW <= 0 || srcH <= 0 || drawW <= 0 || drawH <= 0) return false;

  uint16_t* srcRow = (uint16_t*)malloc((size_t)srcW * sizeof(uint16_t));
  uint16_t* dstRow = (uint16_t*)malloc((size_t)drawW * sizeof(uint16_t));
  if (!srcRow || !dstRow) {
    if (srcRow) free(srcRow);
    if (dstRow) free(dstRow);
    return false;
  }

  int lastSrcY = -1;
  bool ok = true;
  for (int yy = 0; yy < drawH && ok; ++yy) {
    int sy = (yy * srcH) / drawH;
    if (sy < 0) sy = 0;
    if (sy >= srcH) sy = srcH - 1;

    if (sy != lastSrcY) {
      size_t rowOffset = dataOffset + (size_t)sy * (size_t)srcW * sizeof(uint16_t);
      if (!file.seek(rowOffset)) {
        ok = false;
        break;
      }
      size_t needBytes = (size_t)srcW * sizeof(uint16_t);
      size_t readLen = file.read((uint8_t*)srcRow, needBytes);
      if (readLen != needBytes) {
        ok = false;
        break;
      }
      lastSrcY = sy;
    }

    for (int xx = 0; xx < drawW; ++xx) {
      int sx = (xx * srcW) / drawW;
      if (sx < 0) sx = 0;
      if (sx >= srcW) sx = srcW - 1;
      dstRow[xx] = srcRow[sx];
    }

    for (int xx = 0; xx < drawW; ++xx) {
      g.drawPixel(drawX + xx, drawY + yy, dstRow[xx]);
    }
  }

  free(srcRow);
  free(dstRow);
  return ok;
}

static bool drawDressThumbToSprite(LGFX_Sprite& g, fs::FS& fs, const char* thumbPath, int x, int y, int w, int h);

static void fillDressThumbPixels(uint16_t* pixels, int w, int h, uint16_t color) {
  if (!pixels || w <= 0 || h <= 0) return;
  size_t count = (size_t)w * (size_t)h;
  for (size_t i = 0; i < count; ++i) pixels[i] = color;
}

static bool loadDressRaw565ThumbPixels(File& file,
                                       size_t dataOffset,
                                       int srcW,
                                       int srcH,
                                       int dstW,
                                       int dstH,
                                       uint16_t* outPixels) {
  if (!file || !outPixels || srcW <= 0 || srcH <= 0 || dstW <= 0 || dstH <= 0) return false;

  fillDressThumbPixels(outPixels, dstW, dstH, 0xFFFF);
  float scale = dressPngScaleForBox(srcW, srcH, dstW, dstH);
  int drawW = max(1, (int)roundf((float)srcW * scale));
  int drawH = max(1, (int)roundf((float)srcH * scale));
  int drawX = max(0, (dstW - drawW) / 2);
  int drawY = max(0, (dstH - drawH) / 2);

  uint16_t* srcRow = (uint16_t*)malloc((size_t)srcW * sizeof(uint16_t));
  if (!srcRow) return false;

  int lastSrcY = -1;
  bool ok = true;
  for (int yy = 0; yy < drawH && ok; ++yy) {
    int sy = (yy * srcH) / drawH;
    if (sy < 0) sy = 0;
    if (sy >= srcH) sy = srcH - 1;

    if (sy != lastSrcY) {
      size_t rowOffset = dataOffset + (size_t)sy * (size_t)srcW * sizeof(uint16_t);
      if (!file.seek(rowOffset)) {
        ok = false;
        break;
      }
      size_t needBytes = (size_t)srcW * sizeof(uint16_t);
      size_t readLen = file.read((uint8_t*)srcRow, needBytes);
      if (readLen != needBytes) {
        ok = false;
        break;
      }
      lastSrcY = sy;
    }

    uint16_t* dstRow = outPixels + (size_t)(drawY + yy) * (size_t)dstW + (size_t)drawX;
    for (int xx = 0; xx < drawW; ++xx) {
      int sx = (xx * srcW) / drawW;
      if (sx < 0) sx = 0;
      if (sx >= srcW) sx = srcW - 1;
      dstRow[xx] = srcRow[sx];
    }
  }

  free(srcRow);
  return ok;
}

static bool loadDressThumbPixelsViaSprite(fs::FS& fs, const char* thumbPath, int dstW, int dstH, uint16_t* outPixels) {
  if (!thumbPath || !thumbPath[0] || !outPixels || dstW <= 0 || dstH <= 0) return false;

  LGFX_Sprite tmp(&tft);
  tmp.setPsram(true);
  tmp.setColorDepth(16);
  if (!tmp.createSprite(dstW, dstH)) {
    tmp.setPsram(false);
    if (!tmp.createSprite(dstW, dstH)) {
      return false;
    }
  }

  tmp.fillSprite(0xFFFF);
  bool ok = drawDressThumbToSprite(tmp, fs, thumbPath, 0, 0, dstW, dstH);
  if (ok) {
    memcpy(outPixels, tmp.getBuffer(), (size_t)dstW * (size_t)dstH * sizeof(uint16_t));
  }
  tmp.deleteSprite();
  return ok;
}

static bool loadDressThemeThumbWindowEntry(fs::FS& fs,
                                           uint16_t rawIndex,
                                           const char* thumbPath,
                                           int thumbW,
                                           int thumbH,
                                           DressThemeThumbWindowEntry& entry) {
  if (!thumbPath || !thumbPath[0] || thumbW <= 0 || thumbH <= 0) return false;

  size_t pixelCount = (size_t)thumbW * (size_t)thumbH;
  uint16_t* pixels = (uint16_t*)malloc(pixelCount * sizeof(uint16_t));
  if (!pixels) return false;

  bool ok = false;
  if (dressThemeIsRgb565PreviewPath(thumbPath)) {
    File thumbFile = fs.open(thumbPath, FILE_READ);
    if (thumbFile) {
      int srcW = 0;
      int srcH = 0;
      size_t dataOffset = 0;
      if (readDressRgb565Dimensions(thumbFile, srcW, srcH, dataOffset)) {
        ok = loadDressRaw565ThumbPixels(thumbFile, dataOffset, srcW, srcH, thumbW, thumbH, pixels);
      }
      thumbFile.close();
    }
  } else {
    ok = loadDressThumbPixelsViaSprite(fs, thumbPath, thumbW, thumbH, pixels);
  }

  if (!ok) {
    free(pixels);
    Serial.printf("[DRESS] thumb cache load failed: %s\n", thumbPath);
    return false;
  }

  entry.rawIndex = (int16_t)rawIndex;
  entry.w = (uint16_t)thumbW;
  entry.h = (uint16_t)thumbH;
  entry.pixels = pixels;
  return true;
}

static bool ensureDressThemeThumbWindow(fs::FS& fs, int thumbW, int thumbH) {
  dressThemeRebuildFiltered();
  if (dressThemeFilteredCount == 0 || thumbW <= 0 || thumbH <= 0) {
    dressThemeReleaseVisibleThumbCache();
    return false;
  }

  uint16_t windowCount = min<uint16_t>((uint16_t)DRESS_THEME_THUMB_WINDOW_MAX,
                                       min<uint16_t>((uint16_t)dressThemeVisibleSlots(),
                                                     (uint16_t)(dressThemeFilteredCount - dressThemeScroll)));
  bool matches = (dressThemeThumbWindowScroll == dressThemeScroll) &&
                 (dressThemeThumbWindowCount == windowCount) &&
                 (dressThemeThumbWindowW == (uint16_t)thumbW) &&
                 (dressThemeThumbWindowH == (uint16_t)thumbH);
  if (matches) {
    for (uint16_t slot = 0; slot < windowCount; ++slot) {
      uint16_t rawIndex = 0;
      if (!dressThemeRawIndexAtFiltered(dressThemeScroll + slot, rawIndex) ||
          dressThemeThumbWindow[slot].rawIndex != (int16_t)rawIndex ||
          !dressThemeThumbWindow[slot].pixels) {
        matches = false;
        break;
      }
    }
  }
  if (matches) return true;

  dressThemeReleaseVisibleThumbCache();
  dressThemeThumbWindowScroll = dressThemeScroll;
  dressThemeThumbWindowCount = windowCount;
  dressThemeThumbWindowW = (uint16_t)thumbW;
  dressThemeThumbWindowH = (uint16_t)thumbH;

  for (uint16_t slot = 0; slot < windowCount; ++slot) {
    uint16_t rawIndex = 0;
    const char* thumbPath = nullptr;
    if (!dressThemeRawIndexAtFiltered(dressThemeScroll + slot, rawIndex)) continue;
    if (!dressThemeThumbPathAtRawIndex(rawIndex, thumbPath)) continue;
    (void)loadDressThemeThumbWindowEntry(fs, rawIndex, thumbPath, thumbW, thumbH, dressThemeThumbWindow[slot]);
  }
  return true;
}

static const DressThemeThumbWindowEntry* dressThemeThumbWindowEntryAt(uint8_t slot) {
  if (slot >= DRESS_THEME_THUMB_WINDOW_MAX) return nullptr;
  if (slot >= dressThemeThumbWindowCount) return nullptr;
  if (!dressThemeThumbWindow[slot].pixels) return nullptr;
  return &dressThemeThumbWindow[slot];
}

static bool drawDressThumbToSprite(LGFX_Sprite& g, fs::FS& fs, const char* thumbPath, int x, int y, int w, int h) {
  if (!thumbPath || !thumbPath[0]) return false;
  File thumbFile = fs.open(thumbPath, FILE_READ);
  if (!thumbFile) return false;

  int srcW = 0;
  int srcH = 0;
  size_t dataOffset = 0;
  bool isRaw565 = dressThemeIsRgb565PreviewPath(thumbPath);
  bool headerOk = isRaw565
      ? readDressRgb565Dimensions(thumbFile, srcW, srcH, dataOffset)
      : readDressPngDimensions(thumbFile, srcW, srcH);
  if (!headerOk) {
    thumbFile.close();
    Serial.printf("[DRESS] thumb header failed: %s\n", thumbPath);
    return false;
  }
  float scale = dressPngScaleForBox(srcW, srcH, w, h);
  int drawW = (srcW > 0) ? max(1, (int)roundf((float)srcW * scale)) : w;
  int drawH = (srcH > 0) ? max(1, (int)roundf((float)srcH * scale)) : h;
  int drawX = x + max(0, (w - drawW) / 2);
  int drawY = y + max(0, (h - drawH) / 2);

  bool drawn = false;
  if (isRaw565) {
    drawn = drawDressRaw565ThumbToSprite(g, thumbFile, dataOffset, srcW, srcH, drawX, drawY, drawW, drawH);
    if (!drawn) {
      Serial.printf("[DRESS] thumb raw draw failed: %s\n", thumbPath);
    }
  } else {
    size_t fileSize = (size_t)thumbFile.size();
    if (fileSize > 0 && fileSize <= 131072) {
      uint8_t* pngData = (uint8_t*)malloc(fileSize);
      if (!pngData) {
        Serial.printf("[DRESS] thumb png alloc failed: %s (%u bytes)\n", thumbPath, (unsigned int)fileSize);
      } else {
        if (thumbFile.seek(0)) {
          size_t readLen = thumbFile.read(pngData, fileSize);
          if (readLen == fileSize) {
            if (fabsf(scale - 1.0f) < 0.01f) {
              drawn = g.drawPng(pngData, (uint32_t)fileSize, drawX, drawY);
            } else {
              LGFX_Sprite* decodeSprite = nullptr;
              if (ensureDressThemeDecodeSprite(srcW, srcH, decodeSprite) && decodeSprite) {
                decodeSprite->fillSprite(0xFFFF);
                if (decodeSprite->drawPng(pngData, (uint32_t)fileSize, 0, 0)) {
                  drawScaled565ToSprite(g,
                                        (const uint16_t*)decodeSprite->getBuffer(),
                                        srcW,
                                        srcH,
                                        drawX,
                                        drawY,
                                        drawW,
                                        drawH);
                  drawn = true;
                }
              }
            }
          }
        }
        free(pngData);
      }
    }
  }  
  thumbFile.close();
  if (!drawn) {
    Serial.printf("[DRESS] thumb draw failed: %s (%dx%d scale=%.3f)\n", thumbPath, srcW, srcH, scale);
  }
  return drawn;
}

static bool rebuildDressThemePreviewSprite() {
  const uint8_t category = dressThemeCategoryForScene(currentScene);
  uint16_t rawIndex = 0;
  if (!dressThemeAppliedRawIndexForCategory(category, rawIndex)) return false;
  if (rawIndex >= dressThemeCount) return false;

  constexpr int kPreviewW = 226;
  constexpr int kPreviewH = 169;
  if (!ensureDressThemePreviewSprite(kPreviewW, kPreviewH)) return false;
  if (!dressThemePreviewDirty &&
      dressThemePreviewRawIndex == (int16_t)rawIndex &&
      dressThemePreviewCategory == category) {
    return true;
  }

  fs::FS* themeFs = dressThemeAssetFS();
  if (!themeFs) return false;

  const char* thumbPath = nullptr;
  if (!dressThemeThumbPathAtRawIndex(rawIndex, thumbPath)) {
    dressThemePreviewRawIndex = -1;
    dressThemePreviewCategory = 0xFF;
    dressThemePreviewDirty = true;
    return false;
  }
  dressThemePreview.fillSprite(KEY);
  if (!drawDressThumbToSprite(dressThemePreview, *themeFs, thumbPath, 0, 0, kPreviewW, kPreviewH)) {
    dressThemePreviewRawIndex = -1;
    dressThemePreviewCategory = 0xFF;
    dressThemePreviewDirty = true;
    return false;
  }

  dressThemePreviewRawIndex = (int16_t)rawIndex;
  dressThemePreviewCategory = category;
  dressThemePreviewDirty = false;
  return true;
}

static void drawDressThemePreviewBand(int bandY, int bh) {
  if (!rebuildDressThemePreviewSprite()) return;
  int previewX = (SW - dressThemePreviewW) / 2;
  int previewY = max(12, ((SH - UI_BOT_H) - dressThemePreviewH) / 2);
  overlayDressThemePreviewSpriteIntoBand(bandY, bh, previewX, previewY);
}

static void drawDressThemeGalleryDirectBand(uint16_t accent,
                                            int bandY,
                                            int bh,
                                            int galleryX,
                                            int galleryY,
                                            int galleryW,
                                            int galleryH,
                                            int arrowW,
                                            int cardX,
                                            int cardY,
                                            int cardW,
                                            int cardH,
                                            int cardGap,
                                            int labelH,
                                            uint8_t visibleSlots) {
  const uint16_t panelBg = 0xFFFF;
  const uint16_t cardBg = 0xEF7D;
  const uint16_t arrowBg = 0xDEFB;
  const int localGalleryY = galleryY - bandY;

  band.fillRoundRect(galleryX, localGalleryY, galleryW, galleryH, 10, panelBg);
  band.drawRoundRect(galleryX, localGalleryY, galleryW, galleryH, 10, accent);

  auto drawArrow = [&](int x, bool toRight, bool enabled) {
    uint16_t fill = enabled ? panelBg : arrowBg;
    uint16_t fg = enabled ? accent : 0x7BEF;
    int localY = 8 + localGalleryY;
    band.fillRoundRect(x, localY, arrowW - 4, galleryH - 16, 7, fill);
    band.drawRoundRect(x, localY, arrowW - 4, galleryH - 16, 7, fg);
    int cx = x + (arrowW - 4) / 2;
    int cy = localGalleryY + galleryH / 2;
    if (toRight) band.fillTriangle(cx - 4, cy - 6, cx - 4, cy + 6, cx + 4, cy, fg);
    else         band.fillTriangle(cx + 4, cy - 6, cx + 4, cy + 6, cx - 4, cy, fg);
  };

  fs::FS* themeFs = dressThemeAssetFS();
  bool showingComponents = isDressThemeComponentViewActive();
  bool rootReady = dressThemeRootAvailable(dressThemeCategoryForScene(currentScene));
  if (showingComponents) dressThemeRebuildComponents();

  drawArrow(galleryX + 2, false, showingComponents ? dressThemeComponentCanScrollPrev() : dressThemeCanScrollPrev());
  drawArrow(galleryX + galleryW - arrowW + 2, true, showingComponents ? dressThemeComponentCanScrollNext() : dressThemeCanScrollNext());

  bool emptyThemes = (!themeFs || !rootReady || dressThemeFilteredCount == 0);
  bool emptyComponents = (!themeFs || dressThemeComponentCount == 0);
  if ((!showingComponents && emptyThemes) || (showingComponents && emptyComponents)) {
    const char* emptyText = nullptr;
    if (showingComponents) {
      emptyText = themeFs
          ? "\xe8\xaf\xa5\xe4\xb8\xbb\xe9\xa2\x98\xe6\x9a\x82\xe6\x97\xa0\xe7\xbb\x84\xe4\xbb\xb6"
          : "\xe8\xaf\xb7\xe5\x90\x8c\xe6\xad\xa5\xe4\xb8\xbb\xe9\xa2\x98\xe8\xb5\x84\xe6\xba\x90\xe5\x88\xb0SD";
    } else {
      emptyText = (themeFs && rootReady)
          ? "\xe5\xbd\x93\xe5\x89\x8d\xe5\x9c\xba\xe6\x99\xaf\xe6\x9a\x82\xe6\x97\xa0\xe4\xb8\xbb\xe9\xa2\x98"
          : "\xe8\xaf\xb7\xe5\x90\x8c\xe6\xad\xa5\xe4\xb8\xbb\xe9\xa2\x98\xe8\xb5\x84\xe6\xba\x90\xe5\x88\xb0SD";
    }
    drawDressTextOnBand(bandY, galleryX + 34, galleryY + 28, emptyText, TFT_BLACK, panelBg);
    return;
  }

  const int thumbW = cardW - 4;
  const int thumbH = cardH - labelH - 4;
  if (!showingComponents && themeFs) {
    (void)ensureDressThemeThumbWindow(*themeFs, thumbW, thumbH);
  }

  for (uint8_t slot = 0; slot < visibleSlots; ++slot) {
    const char* titleText = nullptr;
    bool selected = false;
    const DressThemeThumbWindowEntry* thumbEntry = nullptr;

    if (showingComponents) {
      uint16_t componentIndex = dressThemeComponentScroll + slot;
      if (componentIndex >= dressThemeComponentCount) break;
      const DressThemeComponentEntry& entry = dressThemeComponents[componentIndex];
      titleText = entry.label;
      selected = (componentIndex == dressThemeComponentSelected);
    } else {
      uint16_t filteredIndex = dressThemeScroll + slot;
      if (filteredIndex >= dressThemeFilteredCount) break;
      uint16_t rawIndex = 0;
      if (!dressThemeRawIndexAtFiltered(filteredIndex, rawIndex)) continue;
      titleText = dressThemes[rawIndex].dirName;
      selected = (filteredIndex == dressThemeSelected);
      thumbEntry = dressThemeThumbWindowEntryAt(slot);
    }

    int xx = 0;
    int yy = 0;
    calcDressThemeGallerySlotRect(slot, cardX, cardY, cardW, cardH, cardGap, visibleSlots, xx, yy);
    int thumbGlobalY = yy + 2;
    int labelY = yy + cardH - labelH;
    uint16_t border = selected ? accent : 0x9CD3;
    uint16_t labelBg = selected ? accent : 0xDEFB;
    uint16_t labelFg = selected ? TFT_WHITE : TFT_BLACK;
    int localCardY = yy - bandY;
    int localLabelY = labelY - bandY;

    band.fillRoundRect(xx, localCardY, cardW, cardH, 8, cardBg);
    band.drawRoundRect(xx, localCardY, cardW, cardH, 8, border);
    band.fillRect(xx + 2, localCardY + 2, cardW - 4, thumbH, 0xFFFF);

    if (showingComponents) {
      const char* imagePath = dressThemeComponents[dressThemeComponentScroll + slot].path;
      if (themeFs && !(thumbGlobalY >= bandY + bh || thumbGlobalY + thumbH <= bandY)) {
        if (!drawDressThumbToSprite(band, *themeFs, imagePath, xx + 2, thumbGlobalY - bandY, cardW - 4, thumbH)) {
          drawDressTextOnBand(bandY, xx + 14, yy + 18, "\xe6\x97\xa0\xe7\xbc\xa9\xe7\x95\xa5\xe5\x9b\xbe", TFT_BLACK, 0xFFFF);
        }
      }
    } else if (thumbEntry) {
      if (thumbGlobalY < bandY + bh && thumbGlobalY + thumbH > bandY) {
        drawDress565PixelsToSprite(band, thumbEntry->pixels, thumbEntry->w, thumbEntry->h, xx + 2, thumbGlobalY - bandY);
      }
    } else {
      drawDressTextOnBand(bandY, xx + 14, yy + 18, "\xe6\x97\xa0\xe7\xbc\xa9\xe7\x95\xa5\xe5\x9b\xbe", TFT_BLACK, 0xFFFF);
    }

    band.fillRoundRect(xx, localLabelY, cardW, labelH, 6, labelBg);
    band.drawRoundRect(xx, localLabelY, cardW, labelH, 6, border);

    char title[96];
    if (showingComponents) dressThemeCopyName(title, sizeof(title), titleText);
    else dressThemeDisplayName(titleText, title, sizeof(title));
    drawDressTextOnBand(bandY, xx + 6, labelY + 3, title, labelFg, labelBg);
  }
}

static bool rebuildDressThemeGallerySprite(uint16_t accent, int galleryX, int galleryY, int galleryW, int galleryH) {
  if (!ensureDressThemeGallerySprite(galleryW, galleryH)) return false;
  if (!dressThemeGalleryDirty) return true;

  dressThemeGallery.fillSprite(KEY);
  dressThemeRebuildFiltered();
  const uint16_t panelBg = 0xFFFF;
  const uint16_t cardBg = 0xEF7D;
  const uint16_t arrowBg = 0xDEFB;

  int arrowW = 0, cardX = 0, cardY = 0, cardW = 0, cardH = 0, cardGap = 0, labelH = 0;
  uint8_t visibleSlots = 0;
  calcDressThemeGalleryLayout(galleryX, galleryY, galleryW, galleryH, arrowW, cardX, cardY, cardW, cardH, cardGap, labelH, visibleSlots);
  const int localCardX = cardX - galleryX;
  const int localCardY0 = cardY - galleryY;
  const int localArrowLeftX = 2;
  const int localArrowRightX = galleryW - arrowW + 2;

  dressThemeGallery.fillRoundRect(0, 0, galleryW, galleryH, 10, panelBg);
  dressThemeGallery.drawRoundRect(0, 0, galleryW, galleryH, 10, accent);

  auto drawArrow = [&](int x, bool toRight, bool enabled) {
    uint16_t fill = enabled ? panelBg : arrowBg;
    uint16_t fg = enabled ? accent : 0x7BEF;
    dressThemeGallery.fillRoundRect(x, 8, arrowW - 4, galleryH - 16, 7, fill);
    dressThemeGallery.drawRoundRect(x, 8, arrowW - 4, galleryH - 16, 7, fg);
    int cx = x + (arrowW - 4) / 2;
    int cy = galleryH / 2;
    if (toRight) dressThemeGallery.fillTriangle(cx - 4, cy - 6, cx - 4, cy + 6, cx + 4, cy, fg);
    else         dressThemeGallery.fillTriangle(cx + 4, cy - 6, cx + 4, cy + 6, cx - 4, cy, fg);
  };

  fs::FS* themeFs = dressThemeAssetFS();
  bool showingComponents = isDressThemeComponentViewActive();
  bool rootReady = dressThemeRootAvailable(dressThemeCategoryForScene(currentScene));
  if (showingComponents) {
    dressThemeRebuildComponents();
  }

  drawArrow(localArrowLeftX, false, showingComponents ? dressThemeComponentCanScrollPrev() : dressThemeCanScrollPrev());
  drawArrow(localArrowRightX, true, showingComponents ? dressThemeComponentCanScrollNext() : dressThemeCanScrollNext());

  bool emptyThemes = (!themeFs || !rootReady || dressThemeFilteredCount == 0);
  bool emptyComponents = (!themeFs || dressThemeComponentCount == 0);
  if ((!showingComponents && emptyThemes) || (showingComponents && emptyComponents)) {
    const char* emptyText = nullptr;
    if (showingComponents) {
      emptyText = themeFs
          ? "\xe8\xaf\xa5\xe4\xb8\xbb\xe9\xa2\x98\xe6\x9a\x82\xe6\x97\xa0\xe7\xbb\x84\xe4\xbb\xb6"
          : "\xe8\xaf\xb7\xe5\x90\x8c\xe6\xad\xa5\xe4\xb8\xbb\xe9\xa2\x98\xe8\xb5\x84\xe6\xba\x90\xe5\x88\xb0SD";
    } else {
      emptyText = (themeFs && rootReady)
          ? "\xe5\xbd\x93\xe5\x89\x8d\xe5\x9c\xba\xe6\x99\xaf\xe6\x9a\x82\xe6\x97\xa0\xe4\xb8\xbb\xe9\xa2\x98"
          : "\xe8\xaf\xb7\xe5\x90\x8c\xe6\xad\xa5\xe4\xb8\xbb\xe9\xa2\x98\xe8\xb5\x84\xe6\xba\x90\xe5\x88\xb0SD";
    }
    drawDressTextOnSprite(dressThemeGallery, 34, 28, emptyText, TFT_BLACK, panelBg);
    dressThemeGalleryDirty = false;
    return true;
  }

  const int thumbW = cardW - 4;
  const int thumbH = cardH - labelH - 4;
  if (!showingComponents && themeFs) {
    (void)ensureDressThemeThumbWindow(*themeFs, thumbW, thumbH);
  }

  for (uint8_t slot = 0; slot < visibleSlots; ++slot) {
    const char* titleText = nullptr;
    bool selected = false;
    const DressThemeThumbWindowEntry* thumbEntry = nullptr;

    if (showingComponents) {
      uint16_t componentIndex = dressThemeComponentScroll + slot;
      if (componentIndex >= dressThemeComponentCount) break;
      const DressThemeComponentEntry& entry = dressThemeComponents[componentIndex];
      titleText = entry.label;
      selected = (componentIndex == dressThemeComponentSelected);
    } else {
      uint16_t filteredIndex = dressThemeScroll + slot;
      if (filteredIndex >= dressThemeFilteredCount) break;

      uint16_t rawIndex = 0;
      if (!dressThemeRawIndexAtFiltered(filteredIndex, rawIndex)) continue;
      titleText = dressThemes[rawIndex].dirName;
      selected = (filteredIndex == dressThemeSelected);
      thumbEntry = dressThemeThumbWindowEntryAt(slot);
    }

    int xx = 0;
    int yy = 0;
    calcDressThemeGallerySlotRect(slot, localCardX, localCardY0, cardW, cardH, cardGap, visibleSlots, xx, yy);
    int labelY = yy + cardH - labelH;
    uint16_t border = selected ? accent : 0x9CD3;
    uint16_t labelBg = selected ? accent : 0xDEFB;
    uint16_t labelFg = selected ? TFT_WHITE : TFT_BLACK;

    dressThemeGallery.fillRoundRect(xx, yy, cardW, cardH, 8, cardBg);
    dressThemeGallery.drawRoundRect(xx, yy, cardW, cardH, 8, border);
    dressThemeGallery.fillRect(xx + 2, yy + 2, cardW - 4, thumbH, 0xFFFF);

    if (showingComponents) {
      const char* imagePath = dressThemeComponents[dressThemeComponentScroll + slot].path;
      if (!drawDressThumbToSprite(dressThemeGallery, *themeFs, imagePath, xx + 2, yy + 2, cardW - 4, thumbH)) {
        drawDressTextOnSprite(dressThemeGallery, xx + 14, yy + 18, "\xe6\x97\xa0\xe7\xbc\xa9\xe7\x95\xa5\xe5\x9b\xbe", TFT_BLACK, 0xFFFF);
      }
    } else if (thumbEntry) {
      drawDress565PixelsToSprite(dressThemeGallery, thumbEntry->pixels, thumbEntry->w, thumbEntry->h, xx + 2, yy + 2);
    } else {
      drawDressTextOnSprite(dressThemeGallery, xx + 14, yy + 18, "\xe6\x97\xa0\xe7\xbc\xa9\xe7\x95\xa5\xe5\x9b\xbe", TFT_BLACK, 0xFFFF);
    }

    dressThemeGallery.fillRoundRect(xx, labelY, cardW, labelH, 6, labelBg);
    dressThemeGallery.drawRoundRect(xx, labelY, cardW, labelH, 6, border);

    char title[96];
    if (showingComponents) {
      dressThemeCopyName(title, sizeof(title), titleText);
    } else {
      dressThemeDisplayName(titleText, title, sizeof(title));
    }
    setCNFont(dressThemeGallery);
    dressThemeGallery.setTextDatum(middle_center);
    dressThemeGallery.setTextColor(labelFg, labelBg);
    dressThemeGallery.drawString(title, xx + cardW / 2, labelY + labelH / 2);
    dressThemeGallery.setTextDatum(top_left);
    resetFont(dressThemeGallery);
  }

  dressThemeGalleryDirty = false;
  return true;
}

static void drawDressThemeGalleryBand(int bandY, int bh, uint16_t accent) {
  dressThemeRebuildFiltered();

  int galleryX = 0, galleryY = 0, galleryW = 0, galleryH = 0;
  int arrowW = 0, cardX = 0, cardY = 0, cardW = 0, cardH = 0, cardGap = 0, labelH = 0;
  uint8_t visibleSlots = 0;
  calcDressThemeGalleryLayout(galleryX, galleryY, galleryW, galleryH, arrowW, cardX, cardY, cardW, cardH, cardGap, labelH, visibleSlots);

  int localGalleryY = galleryY - bandY;
  if (localGalleryY >= band.height() || localGalleryY + galleryH <= 0) return;
  if (!dressThemeGalleryLowMemMode) {
    if (rebuildDressThemeGallerySprite(accent, galleryX, galleryY, galleryW, galleryH)) {
      overlayDressThemeGallerySpriteIntoBand(bandY, bh, galleryX, galleryY);
      return;
    }
    dressThemeGalleryLowMemMode = true;
  }
  drawDressThemeGalleryDirectBand(accent, bandY, bh, galleryX, galleryY, galleryW, galleryH,
                                  arrowW, cardX, cardY, cardW, cardH, cardGap, labelH, visibleSlots);
}

static void drawDressOverlayBand(int bandY, int bh) {
  if (dressThemePanelOpen && dressTab == DRESS_TAB_THEME) {
    const uint16_t accent = dressButtonColor(dressTabButtonIndex(dressTab));
    drawDressThemeGalleryBand(bandY, bh, accent);
  }
}

static void renderDressModeFrame(uint32_t now) {
  (void)now;
  for (int y = 0; y < SH; y += BAND_H) {
    int bh = (y + BAND_H <= SH) ? BAND_H : (SH - y);
    drawDressSceneBand(camX, y, bh);
    drawDressOverlayBand(y, bh);
    overlayUIIntoBand(y, bh);
    pushBandToScreen(y, bh);
  }
}

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
  bool renderingDress = isDressModeActive();
  if (renderingDress != lastRenderedDressMode) {
    lastBandMin = -1;
    lastBandMax = -1;
    lastCamX = camX + 10000.0f;
    uiForceBands = true;
  }

  bool rebuiltUi = false;
  if (uiSpriteDirty) {
    uint8_t nbtn = uiButtonCount();
    if (uiSel >= nbtn) uiSel = 0;
    rebuildUISprites(now);
    uiSpriteDirty = false;
    rebuiltUi = true;
  }

  if (renderingDress) {
    if (!uiForceBands && !rebuiltUi && lastRenderedDressMode) return;
    tft.startWrite();
    renderDressModeFrame(now);
    uiForceBands = false;
    tft.endWrite();
    lastRenderedDressMode = true;
    return;
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

  lastRenderedDressMode = false;
}
