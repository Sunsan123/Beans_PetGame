#pragma once

// Application boot and frame-loop orchestration.

static void appSetup() {
  Serial.begin(115200);
  delay(200);

  storageInit();
  runtimeAssetsInit();
  initAutoBehaviorTable();
  initDailyEventTable();

  audioPwmSetup();
  audioSetTone(0, 0);

  tft.init();
  tft.setRotation(ROT);
#if DISPLAY_PROFILE == DISPLAY_PROFILE_ESP32S3_ST7789
  tft.setBrightness(255);
#elif DISPLAY_PROFILE == DISPLAY_PROFILE_2432S022
  Wire.begin(TOUCH_SDA, TOUCH_SCL);
  Wire.setClock(400000);
  g_touch.init(TOUCH_SDA, TOUCH_SCL, -1, -1);
  tft.setBrightness(255);
#else
  pinMode(PIN_BL, OUTPUT);
  digitalWrite(PIN_BL, HIGH);
  softSPIBeginTouch();
#endif

  SW = tft.width();
  SH = tft.height();

  if (encoderEnabled()) {
    pinMode(ENC_A, INPUT_PULLUP);
    pinMode(ENC_B, INPUT_PULLUP);
  }
  if (encoderButtonEnabled()) pinMode(ENC_BTN, INPUT_PULLUP);
  if (BTN_LEFT >= 0) pinMode(BTN_LEFT, INPUT_PULLUP);
  if (BTN_RIGHT >= 0) pinMode(BTN_RIGHT, INPUT_PULLUP);
  if (BTN_OK >= 0) pinMode(BTN_OK, INPUT_PULLUP);

#if DISPLAY_PROFILE != DISPLAY_PROFILE_2432S022 && DISPLAY_PROFILE != DISPLAY_PROFILE_ESP32S3_ST7789
#if ENABLE_TOUCH_CALIBRATION
  bool touchReady = saveReady ? touchLoadFromSD() : false;
  if (!touchReady) {
    bool canSkip = encoderButtonEnabled() || (BTN_OK >= 0);
    const int COUNTDOWN_SEC = 9;
    bool wantSkip = false;
    bool lastRaw = false;

    if (canSkip) {
      for (int sec = COUNTDOWN_SEC; sec >= 1 && !wantSkip; --sec) {
        tft.fillScreen(TFT_BLACK);
        tft.setTextColor(TFT_WHITE, TFT_BLACK);
        setCNFontTFT();

        tft.setTextSize(2);
        tft.setCursor(10, 18);
        tft.print("\xe4\xb8\x8d\xe7\x94\xa8\xe8\xa7\xa6\xe6\x91\xb8?");

        tft.setTextSize(1);
        tft.setCursor(10, 55);
        tft.print("\xe6\x8c\x89OK/\xe7\xbc\x96\xe7\xa0\x81\xe5\x99\xa8");
        tft.setCursor(10, 68);
        tft.print("\xe7\xa6\x81\xe7\x94\xa8\xe8\xa7\xa6\xe6\x91\xb8\xe5\xb1\x8f");

        tft.setTextSize(2);
        tft.setCursor(10, 102);
        char buf[32];
        snprintf(buf, sizeof(buf), "\xe6\xa0\xa1\xe5\x87\x86\xe5\x80\x92\xe8\xae\xa1\xe6\x97\xb6: %d", sec);
        tft.print(buf);
        resetFontTFT();

        uint32_t t0 = millis();
        while ((uint32_t)(millis() - t0) < 1000UL) {
          bool raw = readButtonRaw(ENC_BTN) || readButtonRaw(BTN_OK);
          if (raw && !lastRaw) {
            delay(20);
            if (readButtonRaw(ENC_BTN) || readButtonRaw(BTN_OK)) { wantSkip = true; break; }
          }
          lastRaw = raw;
          delay(10);
        }
      }
    }

    if (wantSkip) {
      if (saveReady) {
        touchSaveSkipToSD();
        touchReady = touchLoadFromSD();
      } else {
        touchCal.ok = false;
        touchCal.skipped = true;
        touchReady = true;
      }

      tft.fillScreen(TFT_BLACK);
      tft.setTextColor(TFT_WHITE, TFT_BLACK);
      setCNFontTFT();
      tft.setTextSize(2);
      tft.setCursor(10, 60);
      tft.print("\xe8\xa7\xa6\xe6\x91\xb8\xe5\xb1\x8f\xe5\xb7\xb2\xe7\xa6\x81\xe7\x94\xa8");
      resetFontTFT();
      delay(1200);
    } else {
      if (!touchRunCalibrationWizard()) {
        tft.fillScreen(TFT_BLACK);
        tft.setTextColor(TFT_RED);
        setCNFontTFT();
        tft.setTextSize(2);
        tft.setCursor(10, 10);
        tft.print("\xe8\xa7\xa6\xe6\x91\xb8\xe6\xa0\xa1\xe5\x87\x86\xe5\xa4\xb1\xe8\xb4\xa5");
        resetFontTFT();
        while (true) delay(1000);
      }
      if (saveReady) {
        touchReady = touchLoadFromSD();
      } else {
        touchReady = touchCal.ok;
      }
    }
  }
  (void)touchReady;
#endif
#endif

  applySceneConfig(SCENE_DORM, false);

  if (encoderEnabled()) {
    lastAB = readAB();
    attachInterrupt(digitalPinToInterrupt(ENC_A), isrEnc, CHANGE);
    attachInterrupt(digitalPinToInterrupt(ENC_B), isrEnc, CHANGE);
  }

  noInterrupts();
  int32_t p0 = encPos;
  interrupts();
  lastDetent = detentFromEnc(p0);

  randomSeed((uint32_t)(micros() ^ (uint32_t)ESP.getEfuseMac()));

  band.setColorDepth(16);
  if (!band.createSprite(SW, BAND_H)) {
    tft.fillScreen(TFT_BLACK);
    tft.setTextColor(TFT_RED);
    setCNFontTFT();
    tft.setTextSize(2);
    tft.setCursor(10, 10);
    tft.print("\xe5\x86\x85\xe5\xad\x98\xe4\xb8\x8d\xe8\xb6\xb3: \xe8\xaf\xb7\xe9\x99\x8d\xe4\xbd\x8eBAND_H");
    resetFontTFT();
    while (true) delay(1000);
  }

  uiTop.setColorDepth(16);
  uiBot.setColorDepth(16);
  if (!uiTop.createSprite(SW, UI_TOP_H) || !uiBot.createSprite(SW, UI_BOT_H)) {
    tft.fillScreen(TFT_BLACK);
    tft.setTextColor(TFT_RED);
    setCNFontTFT();
    tft.setTextSize(2);
    tft.setCursor(10, 10);
    tft.print("\xe5\x86\x85\xe5\xad\x98\xe4\xb8\x8d\xe8\xb6\xb3: UI\xe7\xb2\xbe\xe7\x81\xb5\xe5\x88\x9b\xe5\xbb\xba\xe5\xa4\xb1\xe8\xb4\xa5");
    resetFontTFT();
    while (true) delay(1000);
  }

  DZ_L = (int)(SW * 0.30f);
  DZ_R = (int)(SW * 0.60f);

  timeSourceInit();

  uint32_t now = millis();

  bool loaded = false;
  if (saveReady) loaded = loadLatestSave(now);

  if (!loaded) {
    offlineInfo = OfflineSettlementInfo{};
    bootOfflineNotice[0] = 0;
    resetToEgg(now);
    if (saveReady) saveNow(now, "boot_new");
  } else {
    processOfflineSettlement(now);
  }
  processDailyEvents(now, true);

  showHomeIntro(now);
  now = millis();
  if (bootOfflineNotice[0] != 0) {
    openOfflineReportModal();
  }
  if (rtcPresent && !rtcTimeValid) {
    if (isModalMode()) rtcPromptModal.pending = true;
    else openRtcPromptModal();
  }
  if (!isModalMode()) maybeOpenPendingDailyEventModal();

  lastPetTick = now;
  nextDailyEventCheckAt = now + 10000UL;

  rebuildUISprites(now);
  uiSpriteDirty = false;

  int dinoX = (int)roundf(worldX - camX);
  int dinoY = (GROUND_Y - TRI_FOOT_Y);

  tft.startWrite();
  renderFrameOptimized(dinoX, dinoY, nullptr, false, 0);
  uiForceBands = false;
  tft.endWrite();
}

static void appLoop() {
  const uint32_t FRAME_MS = 33;
  static uint32_t last = 0;
  uint32_t now = millis();
  if (now - last < FRAME_MS) return;
  last = now;

  saveMaybeEachMinute(now);

  audioTickAlerts(now);
  audioUpdate(now);

  updateButtons(now);

  if (rtcPresent && rtcTimeValid && (nextDailyEventCheckAt == 0 || (int32_t)(now - nextDailyEventCheckAt) >= 0)) {
    processDailyEvents(now, false);
    nextDailyEventCheckAt = now + 10000UL;
  }
  maybeOpenPendingDailyEventModal();

  if (isMiniGameMode()) {
    static bool mgRaw = false;
    static bool mgStable = false;
    static uint32_t mgChangeAt = 0;

    bool raw = false;
    if (ENC_BTN >= 0) raw = (digitalRead(ENC_BTN) == LOW);

    if (raw != mgRaw) { mgRaw = raw; mgChangeAt = now; }
    if ((now - mgChangeAt) > BTN_DEBOUNCE_MS) mgStable = mgRaw;

    static bool mgLastStable = false;
    bool pressedEdge = (mgStable && !mgLastStable);
    mgLastStable = mgStable;
    if ((pressedEdge || btnOkEdge) && appMode == MODE_MG_PLAY) {
      mgJumpRequested = true;
    }

    bool done = mgUpdate(now);
    mgDraw(now);

    if (done) {
      if (mg.kind == TASK_WASH) {
        applyTaskEffects(TASK_WASH, now);
        setMsg(mg.success ? "\xe6\xb8\x85\xe6\xb4\x81\xe5\xae\x8c\xe6\x88\x90!" : "\xe6\xb8\x85\xe6\xb4\x81\xe6\x9c\xaa\xe8\xbe\xbe\xe6\xa0\x87", now, 1500);
      } else if (mg.kind == TASK_PLAY) {
        applyTaskEffects(TASK_PLAY, now);
        setMsg(mg.success ? "\xe9\x99\xaa\xe6\x8a\xa4\xe9\xa1\xba\xe5\x88\xa9!" : "\xe9\x99\xaa\xe6\x8a\xa4\xe6\x9c\xaa\xe8\xbe\xbe\xe6\xa0\x87", now, 1500);
      }

      mg.active = false;
      mg.kind = TASK_NONE;
      appMode = MODE_PET;

      enterState(ST_SIT, now);
      uiSpriteDirty = true;
      uiForceBands = true;
      audioNextAlertAt = 0;
    }
    return;
  }

  if (isModalMode()) {
    handleModalUI(now);
    renderGameFrame(now);
    return;
  }

  if (showMsg && (int32_t)(now - msgUntil) >= 0) {
    showMsg = false;
  }

  if (poopVisible && (int32_t)(now - poopUntil) >= 0) {
    poopVisible = false;
    uiForceBands = true;
  }

  if (!berriesLeftAvailable && berriesRespawnAt != 0 && (int32_t)(now - berriesRespawnAt) >= 0) {
    berriesLeftAvailable = true;
    berriesRespawnAt = 0;
    setMsg("\xe8\xa1\xa5\xe7\xbb\x99\xe7\x82\xb9\xe5\xb7\xb2\xe8\xa1\xa5\xe8\xb4\xa7", now, 1200);
  }

  if (!puddleVisible && puddleRespawnAt != 0 && (int32_t)(now - puddleRespawnAt) >= 0) {
    puddleVisible = true;
    puddleRespawnAt = 0;
    setMsg("\xe9\xa5\xae\xe6\xb0\xb4\xe7\xab\x99\xe5\xb7\xb2\xe8\xa1\xa5\xe6\xb0\xb4", now, 1200);
  }

  if (phase == PHASE_HATCHING) {
    if ((int32_t)(now - hatchNext) >= 0) {
      hatchIdx++;
      hatchNext = now + 220;
      uiForceBands = true;

      if (hatchIdx >= 4) {
        phase = PHASE_ALIVE;
        pet.stage = AGE_JUNIOR;
        pet.evolveProgressMin = 0;

        getRandomBeanName(petName, sizeof(petName));

        char txt[64];
        snprintf(txt, sizeof(txt), "\xe6\xa1\xa3\xe6\xa1\x88\xe7\x99\xbb\xe8\xae\xb0: %s", petName);
        activityVisible = true;
        activityStart = now;
        activityEnd = now + 4500;
        strncpy(activityText, txt, sizeof(activityText) - 1);
        activityText[sizeof(activityText) - 1] = 0;

        enterState(ST_SIT, now);
        saveNow(now, "hatch");
      }
      uiSpriteDirty = true;
    }
  }

  handleTouchUI(now);
  handleEncoderUI(now);
  handleButtonsUI(now);

  if (phase == PHASE_ALIVE) {
    uint32_t tickMs = (uint32_t)(60000UL / (uint32_t)max(1, SIM_SPEED));
    if ((int32_t)(now - lastPetTick) >= (int32_t)tickMs) {
      while ((int32_t)(now - lastPetTick) >= (int32_t)tickMs) {
        lastPetTick += tickMs;
        updatePetTick(now);
      }
    }
  }

  if (task.active) updateTask(now);

  if (!task.active && phase == PHASE_ALIVE) idleUpdate(now);

  audioTickMusic(now);

  if (!task.active && activityVisible && (int32_t)(now - activityEnd) >= 0) {
    activityStopIfFree(now);
    uiSpriteDirty = true;
  }

  if (activityVisible) {
    if (lastActivityUiRefresh == 0) lastActivityUiRefresh = now;
    if ((now - lastActivityUiRefresh) >= ACTIVITY_UI_REFRESH_MS) {
      lastActivityUiRefresh = now;
      uiSpriteDirty = true;
    }
  } else {
    lastActivityUiRefresh = 0;
  }

  static bool lastCritical = false;
  static bool lastBlinkOn = false;
  bool critical = healthCriticalCount() > 0;
  bool blinkOn = critical && ((now / 500UL) % 2U) == 0U;
  if (critical != lastCritical || blinkOn != lastBlinkOn) {
    uiSpriteDirty = true;
    lastCritical = critical;
    lastBlinkOn = blinkOn;
  }

  renderGameFrame(now);
}
