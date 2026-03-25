#pragma once

// Phase 1 refactor:
// Extract offline notice/report + RTC/scene modal flow from the main .ino
// while preserving the existing single-translation-unit build model.

static void buildOfflineNotice() {
  bootOfflineNotice[0] = 0;

  if (!offlineInfo.checked) return;

  if (offlineInfo.skippedNoClock) {
    strncpy(bootOfflineNotice, "\xe5\xbe\x85\xe6\x9c\xba\xe7\xbb\x93\xe7\xae\x97\xe5\xb7\xb2\xe8\xb7\xb3\xe8\xbf\x87: \xe6\x97\xa0RTC\xe6\x97\xb6\xe9\x97\xb4", sizeof(bootOfflineNotice) - 1);
  } else if (offlineInfo.skippedInvalidRtc) {
    strncpy(bootOfflineNotice, "\xe5\xbe\x85\xe6\x9c\xba\xe7\xbb\x93\xe7\xae\x97\xe5\xb7\xb2\xe8\xb7\xb3\xe8\xbf\x87: RTC\xe6\x97\xb6\xe9\x97\xb4\xe6\x97\xa0\xe6\x95\x88", sizeof(bootOfflineNotice) - 1);
  } else if (offlineInfo.skippedNoSavedTime) {
    strncpy(bootOfflineNotice, "\xe5\xbe\x85\xe6\x9c\xba\xe7\xbb\x93\xe7\xae\x97\xe5\xbe\x85\xe5\x91\xbd: \xe9\xa6\x96\xe6\xac\xa1\xe8\xae\xb0\xe5\xbd\x95\xe6\x97\xb6\xe9\x97\xb4", sizeof(bootOfflineNotice) - 1);
  } else if (offlineInfo.skippedInvalidDelta) {
    strncpy(bootOfflineNotice, "\xe5\xbe\x85\xe6\x9c\xba\xe7\xbb\x93\xe7\xae\x97\xe5\xb7\xb2\xe8\xb7\xb3\xe8\xbf\x87: \xe6\x97\xb6\xe9\x97\xb4\xe5\xbc\x82\xe5\xb8\xb8", sizeof(bootOfflineNotice) - 1);
  } else if (offlineInfo.applied) {
    char tags[48] = {0};
    if (offlineInfo.deltaCoin > 0) appendNoticeTag(tags, sizeof(tags), " \xe9\xbe\x99\xe9\x97\xa8\xe5\xb8\x81");
    if (offlineInfo.sleepingMode) appendNoticeTag(tags, sizeof(tags), " \xe4\xbc\x91\xe6\x95\xb4");
    if (offlineInfo.stageChanged) appendNoticeTag(tags, sizeof(tags), " \xe6\x88\x90\xe9\x95\xbf");
    if (offlineInfo.phaseAfter == PHASE_RESTREADY) appendNoticeTag(tags, sizeof(tags), " \xe7\xbb\x93\xe4\xb8\x9a");
    if (offlineInfo.supplyRespawned || offlineInfo.waterRespawned) appendNoticeTag(tags, sizeof(tags), " \xe8\xa1\xa5\xe7\xbb\x99");
    if (offlineInfo.endedCritical && !offlineInfo.petDied) appendNoticeTag(tags, sizeof(tags), " \xe8\x99\x9a\xe5\xbc\xb1");
    if (offlineInfo.petDied) appendNoticeTag(tags, sizeof(tags), " \xe7\xa6\xbb\xe5\xb2\x9b");
    if (tags[0] == 0) appendNoticeTag(tags, sizeof(tags), " \xe5\xb7\xb2\xe7\xbb\x93\xe7\xae\x97");

    snprintf(bootOfflineNotice, sizeof(bootOfflineNotice), "\xe7\xa6\xbb\xe7\xba\xbf%lu\xe5\x88\x86 \xe9\xbe\x99\xe9\x97\xa8\xe5\xb8\x81%+ld%s%s",
             (unsigned long)offlineInfo.appliedMin,
             (long)offlineInfo.deltaCoin,
             offlineInfo.capped ? " \xe5\xb0\x81\xe9\xa1\xb6" : "",
             tags);
  } else {
    strncpy(bootOfflineNotice, "\xe5\xbe\x85\xe6\x9c\xba\xe7\xbb\x93\xe7\xae\x97: \xe6\x97\xa0\xe9\x9c\x80\xe6\x9b\xb4\xe6\x96\xb0", sizeof(bootOfflineNotice) - 1);
  }

  bootOfflineNotice[sizeof(bootOfflineNotice) - 1] = 0;
}

static void setReportLine(uint8_t idx, const char* text) {
  if (idx >= 6) return;
  strncpy(offlineReportModal.lines[idx], text, sizeof(offlineReportModal.lines[idx]) - 1);
  offlineReportModal.lines[idx][sizeof(offlineReportModal.lines[idx]) - 1] = 0;
}

static void openOfflineReportModal() {
  memset(&offlineReportModal, 0, sizeof(offlineReportModal));
  offlineReportModal.active = true;

  if (offlineInfo.skippedNoClock) {
    setReportLine(0, "\xe5\x8d\x9a\xe5\xa3\xab\xe7\xa6\xbb\xe5\xb2\x97\xe6\x8a\xa5\xe5\x91\x8a");
    setReportLine(1, "\xe6\x9c\xaa\xe6\xa3\x80\xe5\x87\xbaRTC\xe6\x97\xb6\xe9\x92\x9f");
    setReportLine(2, "\xe6\x9c\xac\xe6\xac\xa1\xe5\xb7\xb2\xe8\xb7\xb3\xe8\xbf\x87\xe5\xbe\x85\xe6\x9c\xba\xe7\xbb\x93\xe7\xae\x97");
    offlineReportModal.lineCount = 3;
  } else if (offlineInfo.skippedInvalidRtc) {
    setReportLine(0, "\xe5\x8d\x9a\xe5\xa3\xab\xe7\xa6\xbb\xe5\xb2\x97\xe6\x8a\xa5\xe5\x91\x8a");
    setReportLine(1, "\xe5\xb7\xb2\xe6\xa3\x80\xe5\x87\xbaRTC\xe6\xa8\xa1\xe5\x9d\x97");
    setReportLine(2, "\xe4\xbd\x86RTC\xe6\x97\xb6\xe9\x97\xb4\xe6\x97\xa0\xe6\x95\x88");
    setReportLine(3, "\xe8\xaf\xb7\xe5\x85\x88\xe8\xbf\x9b\xe8\xa1\x8cRTC\xe6\xa0\xa1\xe6\x97\xb6");
    offlineReportModal.lineCount = 4;
  } else if (offlineInfo.skippedNoSavedTime) {
    setReportLine(0, "\xe5\x8d\x9a\xe5\xa3\xab\xe7\xa6\xbb\xe5\xb2\x97\xe6\x8a\xa5\xe5\x91\x8a");
    setReportLine(1, "\xe9\xa6\x96\xe6\xac\xa1\xe8\xae\xb0\xe5\xbd\x95RTC\xe6\x97\xb6\xe9\x97\xb4");
    setReportLine(2, "\xe6\x9c\xaa\xe8\xbf\x9b\xe8\xa1\x8c\xe5\x9b\x9e\xe6\xba\xaf\xe7\xbb\x93\xe7\xae\x97");
    offlineReportModal.lineCount = 3;
  } else if (offlineInfo.skippedInvalidDelta) {
    setReportLine(0, "\xe5\x8d\x9a\xe5\xa3\xab\xe7\xa6\xbb\xe5\xb2\x97\xe6\x8a\xa5\xe5\x91\x8a");
    setReportLine(1, "\xe6\xa3\x80\xe6\xb5\x8b\xe5\x88\xb0RTC\xe6\x97\xb6\xe9\x97\xb4\xe5\xbc\x82\xe5\xb8\xb8");
    setReportLine(2, "\xe6\x9c\xac\xe6\xac\xa1\xe5\xb7\xb2\xe8\xb7\xb3\xe8\xbf\x87\xe7\xbb\x93\xe7\xae\x97");
    offlineReportModal.lineCount = 3;
  } else if (offlineInfo.applied) {
    char line0[48];
    char line1[48];
    char line2[48];
    char line3[48];
    char line4[48];
    snprintf(line0, sizeof(line0), "\xe7\xa6\xbb\xe7\xba\xbf:%lum%s%s",
             (unsigned long)offlineInfo.appliedMin,
             offlineInfo.sleepingMode ? " \xe4\xbc\x91\xe6\x95\xb4" : "",
             offlineInfo.capped ? " \xe5\xb0\x81\xe9\xa1\xb6" : "");
    snprintf(line1, sizeof(line1), "\xe9\xa5\xb1\xe8\x85\xb9%+d \xe6\xb0\xb4\xe5\x88\x86%+d \xe5\x8d\xab\xe7\x94\x9f%+d",
             offlineInfo.deltaFaim, offlineInfo.deltaSoif, offlineInfo.deltaHygiene);
    snprintf(line2, sizeof(line2), "\xe5\xbf\x83\xe6\x83\x85%+d \xe7\x96\xb2\xe5\x8a\xb3%+d \xe4\xba\xb2\xe5\xaf\x86%+d",
             offlineInfo.deltaHumeur, offlineInfo.deltaFatigue, offlineInfo.deltaAmour);
    snprintf(line3, sizeof(line3), "\xe5\x81\xa5\xe5\xba\xb7%+d \xe9\xbe\x99\xe9\x97\xa8\xe5\xb8\x81%+ld",
             offlineInfo.deltaSante,
             (long)offlineInfo.deltaCoin);
    if (offlineInfo.petDied) {
      strncpy(line4, "\xe7\xbb\x93\xe6\x9e\x9c:\xe9\xbe\x99\xe6\xb3\xa1\xe6\xb3\xa1\xe5\xb7\xb2\xe7\xa6\xbb\xe5\xb2\x9b", sizeof(line4) - 1);
    } else if (offlineInfo.phaseAfter == PHASE_RESTREADY) {
      strncpy(line4, "\xe7\xbb\x93\xe6\x9e\x9c:\xe6\x88\x90\xe9\x95\xbf\xe8\xae\xb0\xe5\xbd\x95\xe5\xb7\xb2\xe5\xae\x8c\xe6\x88\x90", sizeof(line4) - 1);
    } else if (offlineInfo.stageChanged) {
      strncpy(line4, "\xe7\xbb\x93\xe6\x9e\x9c:\xe9\x98\xb6\xe6\xae\xb5\xe5\xb7\xb2\xe6\x9b\xb4\xe6\x96\xb0", sizeof(line4) - 1);
    } else if (offlineInfo.endedCritical) {
      strncpy(line4, "\xe7\xbb\x93\xe6\x9e\x9c:\xe5\xbd\x93\xe5\x89\x8d\xe5\xa4\x84\xe4\xba\x8e\xe8\x99\x9a\xe5\xbc\xb1", sizeof(line4) - 1);
    } else {
      strncpy(line4, "\xe7\xbb\x93\xe6\x9e\x9c:\xe7\x8a\xb6\xe6\x80\x81\xe5\xb7\xb2\xe5\x90\x8c\xe6\xad\xa5", sizeof(line4) - 1);
    }
    if (offlineInfo.supplyRespawned) strncat(line4, " \xe8\xa1\xa5\xe7\xbb\x99\xe5\xb7\xb2\xe5\xa4\x8d\xe4\xbd\x8d", sizeof(line4) - strlen(line4) - 1);
    if (offlineInfo.waterRespawned) strncat(line4, " \xe9\xa5\xae\xe6\xb0\xb4\xe5\xb7\xb2\xe5\xa4\x8d\xe4\xbd\x8d", sizeof(line4) - strlen(line4) - 1);
    line4[sizeof(line4) - 1] = 0;
    setReportLine(0, line0);
    setReportLine(1, line1);
    setReportLine(2, line2);
    setReportLine(3, line3);
    setReportLine(4, line4);
    offlineReportModal.lineCount = 5;
  } else {
    setReportLine(0, "\xe5\x8d\x9a\xe5\xa3\xab\xe7\xa6\xbb\xe5\xb2\x97\xe6\x8a\xa5\xe5\x91\x8a");
    setReportLine(1, "\xe6\x9c\xac\xe6\xac\xa1\xe6\x97\xa0\xe9\x9c\x80\xe7\xbb\x93\xe7\xae\x97");
    offlineReportModal.lineCount = 2;
  }

  appMode = MODE_MODAL_REPORT;
  uiForceBands = true;
  uiSpriteDirty = true;
}

static void openRtcPromptModal() {
  rtcPromptModal.pending = false;
  rtcPromptModal.sel = 1;
  appMode = MODE_RTC_PROMPT;
  uiForceBands = true;
  uiSpriteDirty = true;
}

static void openRtcSetModal() {
  resetRtcDraftFromBuildTime();
  rtcClampDraft();
  appMode = MODE_RTC_SET;
  uiForceBands = true;
  uiSpriteDirty = true;
}

static void openDailyEventModal() {
  if (!hasPendingDailyEventNotice()) return;
  dailyEventModal.pending = false;
  dailyEventModal.active = true;
  appMode = MODE_MODAL_EVENT;
  uiForceBands = true;
  uiSpriteDirty = true;
}

static void openSceneSelectModal(uint32_t now) {
  if (!canOpenSceneSelect()) {
    setMsg("\xe5\xbd\x93\xe5\x89\x8d\xe7\x8a\xb6\xe6\x80\x81\xe6\x97\xa0\xe6\xb3\x95\xe5\x88\x87\xe6\x8d\xa2\xe5\x9c\xba\xe6\x99\xaf", now, 1600);
    return;
  }
  clearAutoBehavior();
  sceneModal.active = true;
  sceneModal.sel = currentScene;
  appMode = MODE_SCENE_SELECT;
  uiForceBands = true;
  uiSpriteDirty = true;
}

static void closeModalToPet(uint32_t now) {
  (void)now;
  offlineReportModal.active = false;
  dailyEventModal.active = false;
  sceneModal.active = false;
  if (rtcPromptModal.pending) {
    openRtcPromptModal();
    return;
  }
  if (hasPendingDailyEventNotice()) {
    openDailyEventModal();
    return;
  }
  appMode = MODE_PET;
  uiForceBands = true;
  uiSpriteDirty = true;
}

static void adjustRtcDraftField(int delta) {
  rtcClampDraft();
  switch (rtcDraft.field) {
    case 0: rtcDraft.year = clampi(rtcDraft.year + delta, 2024, 2099); break;
    case 1:
      rtcDraft.month += delta;
      if (rtcDraft.month < 1) rtcDraft.month = 12;
      if (rtcDraft.month > 12) rtcDraft.month = 1;
      break;
    case 2: {
      int maxDay = (int)daysInMonth(rtcDraft.year, rtcDraft.month);
      rtcDraft.day += delta;
      if (rtcDraft.day < 1) rtcDraft.day = maxDay;
      if (rtcDraft.day > maxDay) rtcDraft.day = 1;
      break;
    }
    case 3:
      rtcDraft.hour += delta;
      if (rtcDraft.hour < 0) rtcDraft.hour = 23;
      if (rtcDraft.hour > 23) rtcDraft.hour = 0;
      break;
    case 4:
      rtcDraft.minute += delta;
      if (rtcDraft.minute < 0) rtcDraft.minute = 59;
      if (rtcDraft.minute > 59) rtcDraft.minute = 0;
      break;
    default: break;
  }
  rtcClampDraft();
  uiForceBands = true;
  uiSpriteDirty = true;
}

static void modalActionLeft(uint32_t now) {
  if (appMode == MODE_MODAL_REPORT) {
    closeModalToPet(now);
    return;
  }
  if (appMode == MODE_MODAL_EVENT) {
    popDailyEventNotice();
    if (saveReady) saveNow(now, "event_ack");
    closeModalToPet(now);
    return;
  }
  if (appMode == MODE_RTC_PROMPT) {
    rtcPromptModal.sel = 0;
    uiForceBands = true;
    uiSpriteDirty = true;
    return;
  }
  if (appMode == MODE_RTC_SET) {
    adjustRtcDraftField(-1);
    return;
  }
  if (appMode == MODE_SCENE_SELECT) {
    int s = (int)sceneModal.sel - 1;
    if (s < 0) s = (int)SCENE_COUNT - 1;
    sceneModal.sel = (SceneId)s;
    uiForceBands = true;
    uiSpriteDirty = true;
  }
}

static void modalActionRight(uint32_t now) {
  if (appMode == MODE_MODAL_REPORT) {
    closeModalToPet(now);
    return;
  }
  if (appMode == MODE_MODAL_EVENT) {
    popDailyEventNotice();
    if (saveReady) saveNow(now, "event_ack");
    closeModalToPet(now);
    return;
  }
  if (appMode == MODE_RTC_PROMPT) {
    rtcPromptModal.sel = 1;
    uiForceBands = true;
    uiSpriteDirty = true;
    return;
  }
  if (appMode == MODE_RTC_SET) {
    adjustRtcDraftField(+1);
    return;
  }
  if (appMode == MODE_SCENE_SELECT) {
    int s = ((int)sceneModal.sel + 1) % (int)SCENE_COUNT;
    sceneModal.sel = (SceneId)s;
    uiForceBands = true;
    uiSpriteDirty = true;
  }
}

static void modalActionOk(uint32_t now) {
  if (appMode == MODE_MODAL_REPORT) {
    closeModalToPet(now);
    return;
  }
  if (appMode == MODE_MODAL_EVENT) {
    popDailyEventNotice();
    if (saveReady) saveNow(now, "event_ack");
    closeModalToPet(now);
    return;
  }

  if (appMode == MODE_RTC_PROMPT) {
    if (rtcPromptModal.sel == 0) {
      closeModalToPet(now);
    } else {
      openRtcSetModal();
    }
    return;
  }

  if (appMode == MODE_RTC_SET) {
    if (rtcDraft.field < 4) {
      rtcDraft.field++;
      uiForceBands = true;
      uiSpriteDirty = true;
      return;
    }

    if (rtcWriteDraftToChip()) {
      if (saveReady) saveNow(now, "rtc_set");
      setMsg("\xe6\x97\xb6\xe9\x92\x9f\xe6\xa0\xa1\xe6\x97\xb6\xe5\xb7\xb2\xe5\xae\x8c\xe6\x88\x90", now, 1800);
      closeModalToPet(now);
    } else {
      setMsg("\xe6\x97\xb6\xe9\x92\x9f\xe5\x86\x99\xe5\x85\xa5\xe5\xa4\xb1\xe8\xb4\xa5", now, 1800);
      appMode = MODE_PET;
    }
    return;
  }

  if (appMode == MODE_SCENE_SELECT) {
    char tmp[48];
    applySceneConfig(sceneModal.sel, false);
    clearAutoBehavior();
    enterState(ST_SIT, now);
    snprintf(tmp, sizeof(tmp), "\xe5\xb7\xb2\xe5\x88\x87\xe6\x8d\xa2\xe8\x87\xb3%s", sceneLabel(currentScene));
    setMsg(tmp, now, 1600);
    if (saveReady) saveNow(now, "scene");
    closeModalToPet(now);
  }
}

static void handleModalUI(uint32_t now) {
  if (!isModalMode()) return;

  if (encoderEnabled()) {
    int32_t p;
    noInterrupts(); p = encPos; interrupts();
    int32_t det = detentFromEnc(p);
    int32_t dd = det - lastDetent;
    if (dd < 0) modalActionLeft(now);
    if (dd > 0) modalActionRight(now);
    if (dd != 0) lastDetent = det;

    bool raw = readBtnPressedRaw();
    if (raw != lastBtnRaw) { lastBtnRaw = raw; btnChangeAt = now; }
    if ((now - btnChangeAt) > BTN_DEBOUNCE_MS) btnState = raw;

    static bool modalLastStable = false;
    bool pressedEdge = (btnState && !modalLastStable);
    modalLastStable = btnState;
    if (pressedEdge) modalActionOk(now);
  }

  if (buttonsEnabled()) {
    if (btnLeftEdge) modalActionLeft(now);
    if (btnRightEdge) modalActionRight(now);
    if (btnOkEdge) modalActionOk(now);
  }

  int16_t x = 0, y = 0;
  bool down = readTouchRaw(x, y);
  if (down != modalTouch.rawDown) {
    modalTouch.rawDown = down;
    modalTouch.lastChange = now;
    if (down) { modalTouch.x = x; modalTouch.y = y; }
  } else if (down) {
    modalTouch.x = x;
    modalTouch.y = y;
  }

  bool stableDown = modalTouch.rawDown && (now - modalTouch.lastChange) >= MODAL_TOUCH_DEBOUNCE_MS;
  bool stableUp   = (!modalTouch.rawDown) && (now - modalTouch.lastChange) >= MODAL_TOUCH_DEBOUNCE_MS;
  bool pressedEdge = false;
  if (!modalTouch.stableDown && stableDown) { modalTouch.stableDown = true; pressedEdge = true; }
  if (modalTouch.stableDown && stableUp) { modalTouch.stableDown = false; }
  if (!pressedEdge) return;

  x = modalTouch.x;
  y = modalTouch.y;

  if (appMode == MODE_MODAL_REPORT) {
    modalActionOk(now);
    return;
  }
  if (appMode == MODE_MODAL_EVENT) {
    modalActionOk(now);
    return;
  }

  if (appMode == MODE_RTC_PROMPT) {
    if (y >= 96 && y <= 140) {
      rtcPromptModal.sel = (x < (SW / 2)) ? 0 : 1;
      uiForceBands = true;
      uiSpriteDirty = true;
      return;
    }
  }

  if (appMode == MODE_RTC_SET) {
    if (y >= 74 && y < 74 + 5 * 28) {
      rtcDraft.field = (uint8_t)clampi((y - 74) / 28, 0, 4);
      uiForceBands = true;
      uiSpriteDirty = true;
      return;
    }
  }

  if (appMode == MODE_SCENE_SELECT) {
    if (y >= 90 && y < 90 + (int)SCENE_COUNT * 28) {
      sceneModal.sel = (SceneId)clampi((y - 90) / 28, 0, (int)SCENE_COUNT - 1);
      uiForceBands = true;
      uiSpriteDirty = true;
      return;
    }
  }

  if (y >= SH - 54) {
    if (x < SW / 3) modalActionLeft(now);
    else if (x > (SW * 2) / 3) modalActionRight(now);
    else modalActionOk(now);
  } else if (appMode == MODE_RTC_PROMPT) {
    modalActionOk(now);
  }
}
