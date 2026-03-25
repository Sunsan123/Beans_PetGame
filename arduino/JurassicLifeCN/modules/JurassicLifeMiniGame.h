#pragma once

// Mini-game state updates and rendering.
// Extracted from JurassicLifeCN.ino during the refactor.

// ================== MINI-JEUX (stubs pr闂佹寧顒畇 闁?remplacer) ==================
static void mgBegin(TaskKind k, uint32_t now) {
  if (k != TASK_WASH && k != TASK_PLAY) return;

  mg.active = true;
  mg.kind = k;
  mg.startedAt = now;
  mg.success = true;
  mg.score = 0;

  appMode = (k == TASK_WASH) ? MODE_MG_WASH : MODE_MG_PLAY;

  // couper barre activit闁?de la gestion
  activityVisible = false;
  activityText[0] = 0;
  showMsg = false;

  // stop idle anim pour 闂佹寧顒畆e clean au retour
  enterState(ST_SIT, now);

  for (int i = 0; i < MG_RAIN_MAX; i++) mgRain[i] = RainDrop{};
  for (int i = 0; i < MG_BALLOON_MAX; i++) mgBalloons[i] = Balloon{};

  mgCloudX = (float)(SW / 2);
  mgCloudV = 0.9f;
  mgDinoX = (float)(SW / 2);
  mgDinoV = 2.2f;
  mgDropsHit = 0;

  mgPlayDinoY = 0.0f;
  mgPlayDinoVy = 0.0f;
  mgBalloonsCaught = 0;
  mgBalloonsSpawned = 0;
  mgNextBalloonAt = now + 600;
  mgJumpRequested = false;
  mgNextDropAt = now; // pluie active d闁绘氨骞?le d闁肩厧宕憉t du mini-jeu laver
  mgNeedsFullRedraw = true;
  mgPrevCloudX = mgCloudX;
  mgPrevDinoX = mgDinoX;
  mgPrevWashDinoY = SH - UI_BOT_H - triH(pet.stage) - 4;
  mgPrevPlayDinoTop = SH - UI_BOT_H - 6 - triH(pet.stage);
  mgAnimIdx = 0;
  mgAnimNextTick = now + 120;
  noInterrupts();
  mgLastDetent = detentFromEnc(encPos);
  interrupts();

  tft.fillScreen(TFT_BLACK);
}

static bool mgUpdate(uint32_t now) {
  if (mg.startedAt == 0) { mg.success = false; mg.score = 0; return true; }

  int cloudW = 0;
  int cloudH = 0;
  runtimeSingleDimensions(BeansAssets::AssetId::SceneCommonPropCloud, cloudW, cloudH);
  int balloonW = 0;
  int balloonH = 0;
  runtimeSingleDimensions(BeansAssets::AssetId::SceneCommonPropBalloon, balloonW, balloonH);

  if ((int32_t)(now - mgAnimNextTick) >= 0) {
    uint8_t animId = animIdForState(pet.stage, ST_WALK);
    uint8_t cnt = triAnimCount(pet.stage, animId);
    if (cnt == 0) cnt = 1;
    mgAnimIdx = (uint8_t)((mgAnimIdx + 1) % cnt);
    mgAnimNextTick = now + 120;
  }

  if (mg.kind == TASK_WASH) {
    if (cloudW <= 12 || cloudH <= 0) return false;
    const int DINO_DRAW_W = triW(pet.stage);
    const int DINO_HIT_W = triW(pet.stage);
    const int DINO_HIT_H = triH(pet.stage);
    const int DINO_Y_HIT = SH - UI_BOT_H - DINO_HIT_H - 4 + 25;

    int16_t tx = 0, ty = 0;
    if (readTouchScreen(tx, ty)) {
      (void)ty;
      mgCloudX = (float)tx;
    }

    int32_t encNow;
    noInterrupts();
    encNow = encPos;
    interrupts();
    int32_t det = detentFromEnc(encNow);
    int32_t dd = det - mgLastDetent;
    if (dd != 0) {
      mgCloudX += (float)dd * 6.0f;
      mgLastDetent = det;
    }

    if (btnLeftHeld) mgCloudX -= 4.0f;
    if (btnRightHeld) mgCloudX += 4.0f;

    mgCloudX += mgCloudV;
    float cloudHalf = (float)(cloudW / 2);
    if (mgCloudX < cloudHalf) { mgCloudX = cloudHalf; mgCloudV = fabsf(mgCloudV); }
    if (mgCloudX > (float)(SW - cloudHalf)) { mgCloudX = (float)(SW - cloudHalf); mgCloudV = -fabsf(mgCloudV); }

    mgDinoX += mgDinoV;
    if (mgDinoX < -20) { mgDinoX = -20; mgDinoV = fabsf(mgDinoV); }
    if (mgDinoX > SW - DINO_DRAW_W + 20) { mgDinoX = (float)(SW - DINO_DRAW_W + 20); mgDinoV = -fabsf(mgDinoV); }

// --- spawn pluie bas闁?sur le temps (NE S'ARRETE PAS) ---
if ((int32_t)(now - mgNextDropAt) >= 0) {

  // 1 ou 2 gouttes 闁?chaque tick (pluie plus vivante)
  int toSpawn = 1 + (random(0, 100) < 35 ? 1 : 0);

  while (toSpawn-- > 0) {
    for (int i = 0; i < MG_RAIN_MAX; i++) {
      if (!mgRain[i].active) {
        mgRain[i].active = true;
        mgRain[i].x = mgCloudX + (float)random(-cloudW / 2 + 6, cloudW / 2 - 6);
        mgRain[i].y = (float)cloudH + 4;
        mgRain[i].vy = 2.6f + (float)random(0, 10) * 0.1f;
        break;
      }
    }
  }

  // intervalle pluie : plus petit = plus de pluie
  mgNextDropAt = now + (uint32_t)random(35, 65);
}


    for (int i = 0; i < MG_RAIN_MAX; i++) {
      if (!mgRain[i].active) continue;
      mgRain[i].y += mgRain[i].vy;
      if (mgRain[i].y > (float)(SH - UI_BOT_H)) {
        mgRain[i].active = false;
        continue;
      }
      float rx = mgRain[i].x;
      float ry = mgRain[i].y;
      if (rx >= mgDinoX && rx <= (mgDinoX + DINO_HIT_W) &&
          ry >= (float)DINO_Y_HIT && ry <= (float)(DINO_Y_HIT + DINO_HIT_H)) {
        mgRain[i].active = false;
        mgDropsHit++;
        playRTTTLOnce(RTTTL_RAIN_HIT_SFX, AUDIO_PRIO_LOW);
      }
    }

    mg.score = mgDropsHit;
    if (mgDropsHit >= 50) {
      mg.success = true;
      return true;
    }
    return false;
  }

  if (mg.kind == TASK_PLAY) {
    const int DINO_X = 14;
    const int DINO_W = triW(pet.stage);
    const int DINO_H = triH(pet.stage);
    const int GROUND = SH - UI_BOT_H - 1;
    const int BALLOON_MIN_Y = 70;
    const int BALLOON_MAX_Y = max(BALLOON_MIN_Y, GROUND - DINO_H + 10);
    const float GRAV = 0.65f;
    const float JUMP_V0 = -9.5f;

    int16_t tx = 0, ty = 0;
    if (readTouchScreen(tx, ty)) {
      if (ty < (SH - UI_BOT_H) && mgPlayDinoY == 0.0f) {
        mgPlayDinoVy = JUMP_V0;
      }
    }

    if (mgJumpRequested) {
      if (mgPlayDinoY == 0.0f) {
        mgPlayDinoVy = JUMP_V0;
      }
      mgJumpRequested = false;
    }

    mgPlayDinoVy += GRAV;
    mgPlayDinoY += mgPlayDinoVy;
    if (mgPlayDinoY > 0.0f) {
      mgPlayDinoY = 0.0f;
      mgPlayDinoVy = 0.0f;
    }

    if (mgBalloonsCaught < 10 && (int32_t)(now - mgNextBalloonAt) >= 0) {
      for (int i = 0; i < MG_BALLOON_MAX; i++) {
        if (!mgBalloons[i].active) {
          mgBalloons[i].active = true;
          mgBalloons[i].x = (float)(SW + 10);
          mgBalloons[i].y = (float)random(BALLOON_MIN_Y, BALLOON_MAX_Y);
          mgBalloons[i].vx = -(2.6f + (float)random(0, 20) * 0.1f);
          mgBalloonsSpawned++;
          mgNextBalloonAt = now + (uint32_t)random(900, 1600
        );
          break;
        }
      }
    }

    int dinoTop = (int)(GROUND - DINO_H + mgPlayDinoY + 25);
    for (int i = 0; i < MG_BALLOON_MAX; i++) {
      if (!mgBalloons[i].active) continue;
      mgBalloons[i].x += mgBalloons[i].vx;
      if (mgBalloons[i].x < -20) {
        mgBalloons[i].active = false;
        continue;
      }
      float bx = mgBalloons[i].x;
      float by = mgBalloons[i].y;
      const float r = 8.0f;
      if (bx + r >= (float)DINO_X && bx - r <= (float)(DINO_X + DINO_W) &&
          by + r >= (float)dinoTop && by - r <= (float)(dinoTop + DINO_H)) {
        mgBalloons[i].active = false;
        mgBalloonsCaught++;
        playRTTTLOnce(RTTTL_BALLOON_CATCH_SFX, AUDIO_PRIO_LOW);
      }
    }

    mg.score = mgBalloonsCaught;
    if (mgBalloonsCaught >= 10) {
      mg.success = true;
      return true;
    }
  }

  return false;
}

static void mgDraw(uint32_t now) {
  (void)now;
  int cloudW = 0;
  int cloudH = 0;
  runtimeSingleDimensions(BeansAssets::AssetId::SceneCommonPropCloud, cloudW, cloudH);
  int balloonW = 0;
  int balloonH = 0;
  runtimeSingleDimensions(BeansAssets::AssetId::SceneCommonPropBalloon, balloonW, balloonH);
  tft.startWrite();
  for (int y = 0; y < SH; y += BAND_H) {
    int bh = (y + BAND_H <= SH) ? BAND_H : (SH - y);
    band.fillRect(0, 0, SW, bh, MG_SKY);
    band.setClipRect(0, 0, SW, bh);

    if (appMode == MODE_MG_WASH) {
const int GROUND = SH - UI_BOT_H - 6;      // <-- sol du mini-jeu
const int DINO_W = triW(pet.stage);
const int DINO_H = triH(pet.stage);
const int DINO_Y = GROUND - DINO_H + 25;        // <-- dino pos闁?sur le sol

const uint8_t animIdWalk = (uint8_t)animIdForState(pet.stage, ST_WALK);
bool flip = flipForMovingRight(mgDinoV >= 0.0f);

// ---- SOL (闁?ajouter) ----
if (y + bh > GROUND) {
  int gy0 = max(GROUND, y);
  int gy1 = min(SH, y + bh);
  if (gy1 > gy0) band.fillRect(0, gy0 - y, SW, gy1 - gy0, MG_GROUND);
}
if (GROUND >= y && GROUND < y + bh) {
  band.drawFastHLine(0, GROUND - y, SW, MG_GLINE);
}


      if (y + bh > 0 && y < 22) {
        setCNFont(band);
        band.setTextColor(TFT_WHITE, MG_SKY);
        band.setTextSize(1);
        band.setCursor(6, 4 - y);
        band.print("\xe5\x90\x8e\xe5\x8b\xa4\xe4\xbb\xbb\xe5\x8a\xa1-\xe6\xb8\x85\xe6\xb4\x81");  // 闂佸憡鑹炬鍝モ偓姘卞枑缁傛帞鎷犻幓鎺濇匠-濠电偞鎸搁幊蹇曞?
        resetFont(band);
      }

      if (cloudW > 0 && cloudH > 0 && y + bh > 4 && y < 4 + cloudH) {
        drawRuntimeSingleAssetOnBand(BeansAssets::AssetId::SceneCommonPropCloud, (int)mgCloudX - cloudW / 2, 4 - y, false, 0);
      }

      for (int i = 0; i < MG_RAIN_MAX; i++) {
        if (!mgRain[i].active) continue;
        int ry = (int)mgRain[i].y;
        int y0 = ry - y;
        if (y0 + 6 < 0 || y0 >= bh) continue;
        band.drawFastVLine((int)mgRain[i].x, y0, 6, 0x001F);
      }

      if (y + bh > DINO_Y && y < DINO_Y + DINO_H) {
        drawTriceratopsFrameOnBand(pet.stage, animIdWalk, mgAnimIdx, nullptr, (int)mgDinoX, DINO_Y - y, flip, 0);
      }
    } 
    else {
      const int DINO_X = 14;
      const int DINO_W = triW(pet.stage);
      const int DINO_H = triH(pet.stage);
      const int GROUND = SH - UI_BOT_H - 6;
      int dinoTop = (int)(GROUND - DINO_H + mgPlayDinoY + 25);
      const uint8_t animIdWalk = (uint8_t)animIdForState(pet.stage, ST_WALK);

      if (y + bh > 0 && y < 22) {
        setCNFont(band);
        band.setTextColor(TFT_WHITE, MG_SKY); // fond = ciel (ou mets juste TFT_WHITE)
        band.setTextSize(1);
        band.setCursor(6, 4 - y);
        band.print("\xe9\x99\xaa\xe6\x8a\xa4\xe4\xbb\xbb\xe5\x8a\xa1-\xe7\x8e\xa9\xe8\x80\x8d");  // 闂傚倸瀚锋禍婵囨叏閵忥紕顩烽悹鍥ㄥ絻椤?闂佺粯澹曢弲鐐哄焵?
        resetFont(band);
      }

// Remplir le bas en herbe 闁?partir du sol (fixe, pas li闁?au saut)
if (y + bh > GROUND) {
  int gy0 = max(GROUND, y);
  int gy1 = min(SH, y + bh);
  if (gy1 > gy0) {
    band.fillRect(0, gy0 - y, SW, gy1 - gy0, MG_GROUND);
  }
}

// Ligne du sol par-dessus (optionnel mais joli)
if (GROUND >= y && GROUND < y + bh) {
  band.drawFastHLine(0, GROUND - y, SW, MG_GLINE);
}

      for (int i = 0; i < MG_BALLOON_MAX; i++) {
        if (!mgBalloons[i].active) continue;
        int bx = (int)mgBalloons[i].x - balloonW / 2;
        int by = (int)mgBalloons[i].y - balloonH / 2;
        if (balloonW > 0 && balloonH > 0 && y + bh > by && y < by + balloonH) {
          drawRuntimeSingleAssetOnBand(BeansAssets::AssetId::SceneCommonPropBalloon, bx, by - y, false, 0);
        }
      }

      if (y + bh > dinoTop && y < dinoTop + DINO_H) {
        drawTriceratopsFrameOnBand(pet.stage, animIdWalk, mgAnimIdx, nullptr, DINO_X, dinoTop - y, flipForMovingRight(true), 0);
      }
    }

// ===== HUD bas mini-jeu : SUR VERT, PAS DE NOIR (clipp闁? =====
const int HUD_TOP = SH - UI_BOT_H;

if (y + bh > HUD_TOP) {
  // Remplir uniquement la partie HUD qui est dans cette bande (jamais de y n闁肩厧宕tif)
  int y0 = max(HUD_TOP, y);
  int y1 = min(SH, y + bh);
  int h  = y1 - y0;
  if (h > 0) {
    band.fillRect(0, y0 - y, SW, h, MG_GROUND);
  }

  // Texte (ombre + blanc) SANS fond
  setCNFont(band);
  band.setTextSize(1);

  int ty1 = (HUD_TOP + 6) - y;   // coord dans la bande
  int ty2 = (HUD_TOP + 20) - y;

  // ombre
  band.setTextColor(TFT_BLACK);
  band.setCursor(7, ty1 + 1);
  if (appMode == MODE_MG_WASH) {
    band.print("\xe6\xb8\x85\xe6\xb4\x81\xe8\xbf\x9b\xe5\xba\xa6: "); band.print(mgDropsHit); band.print(" / 50");       // 濠电偞鎸搁幊蹇曞閵夛附浜ゆ繛鎴灻?
  } else {
    band.print("\xe6\xb0\x94\xe6\xb3\xa1: "); band.print(mgBalloonsCaught); band.print(" / 10");  // 濠殿喗蓱濮婂綊宕?
  }
  band.setCursor(7, ty2 + 1);
  band.print("\xe5\xae\x8c\xe6\x88\x90\xe4\xbb\xbb\xe5\x8a\xa1\xe5\x90\x8e\xe8\xbf\x94\xe5\x9b\x9e");  // 闁诲海鎳撻張顒勫垂濮橆厾顩烽悹鍥ㄥ絻椤倝鏌涘顒佸攭缂佺粯鍨垮畷?

  // texte
  band.setTextColor(TFT_WHITE);
  band.setCursor(6, ty1);
  if (appMode == MODE_MG_WASH) {
    band.print("\xe6\xb8\x85\xe6\xb4\x81\xe8\xbf\x9b\xe5\xba\xa6: "); band.print(mgDropsHit); band.print(" / 50");       // 濠电偞鎸搁幊蹇曞閵夛附浜ゆ繛鎴灻?
  } else {
    band.print("\xe6\xb0\x94\xe6\xb3\xa1: "); band.print(mgBalloonsCaught); band.print(" / 10");  // 濠殿喗蓱濮婂綊宕?
  }
  band.setCursor(6, ty2);
  band.print("\xe5\xae\x8c\xe6\x88\x90\xe4\xbb\xbb\xe5\x8a\xa1\xe5\x90\x8e\xe8\xbf\x94\xe5\x9b\x9e");  // 闁诲海鎳撻張顒勫垂濮橆厾顩烽悹鍥ㄥ絻椤倝鏌涘顒佸攭缂佺粯鍨垮畷?
  resetFont(band);
}


    tft.pushImage(0, y, SW, bh, (uint16_t*)band.getBuffer());


  }

  tft.endWrite();
  mgNeedsFullRedraw = false;
}
