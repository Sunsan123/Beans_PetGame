#pragma once

// Offline settlement, daily events, and idle/auto-behavior flow.
// Extracted from JurassicLifeCN.ino during the refactor.

static void applyOfflineMinuteTick(uint32_t now, bool sleeping) {
  if (!pet.vivant) return;
  if (phase != PHASE_ALIVE) return;

  auto add = [](float &v, float dv){ v = clamp01f(v + dv); };

  if (sleeping) {
    add(pet.faim,    -1.0f);
    add(pet.soif,    -1.0f);
    add(pet.hygiene, -0.5f);
    add(pet.humeur,  +0.2f);
    add(pet.fatigue, SLEEP_GAIN_PER_SEC * 60.0f);
    add(pet.amour,   -0.2f);
    add(pet.sante,   +0.1f);
  } else {
    add(pet.faim,    AWAKE_HUNGER_D);
    add(pet.soif,    AWAKE_THIRST_D);
    add(pet.hygiene, AWAKE_HYGIENE_D);
    add(pet.humeur,  AWAKE_MOOD_D);
    add(pet.fatigue, AWAKE_FATIGUE_D);
    add(pet.amour,   AWAKE_LOVE_D);
  }

  updateHealthTick(now);

  if (!pet.vivant || phase != PHASE_ALIVE) return;

  pet.lungmenCoin += LUNGMEN_COIN_PER_MIN;
  pet.ageMin++;
  evolutionTick(now);

  uiSpriteDirty = true;
  uiForceBands  = true;
}

static void processOfflineSettlement(uint32_t now) {
  offlineInfo = OfflineSettlementInfo{};
  offlineInfo.checked = true;

  uint32_t currentUnixTime = 0;
  if (!readCurrentUnixTime(currentUnixTime)) {
    offlineInfo.skippedNoClock = !rtcPresent;
    offlineInfo.skippedInvalidRtc = rtcPresent;
    buildOfflineNotice();
    return;
  }

  if (!savedTimeValid || savedLastUnixTime == 0) {
    offlineInfo.skippedNoSavedTime = true;
    buildOfflineNotice();
    if (saveReady) saveNow(now, "boot_time_seed");
    return;
  }

  if (currentUnixTime <= savedLastUnixTime) {
    offlineInfo.skippedInvalidDelta = true;
    buildOfflineNotice();
    if (saveReady) saveNow(now, "boot_time_reset");
    return;
  }

  offlineInfo.elapsedSec = currentUnixTime - savedLastUnixTime;
  uint32_t elapsedMin = offlineInfo.elapsedSec / 60UL;
  if (elapsedMin == 0) {
    buildOfflineNotice();
    return;
  }

  if (!pet.vivant || phase == PHASE_TOMB || phase == PHASE_RESTREADY) {
    buildOfflineNotice();
    if (saveReady) saveNow(now, "boot_phase_hold");
    return;
  }

  offlineInfo.appliedMin = min(elapsedMin, OFFLINE_SETTLE_CAP_MIN);
  offlineInfo.capped = (offlineInfo.appliedMin < elapsedMin);
  offlineInfo.sleepingMode = savedWasSleeping;
  offlineInfo.stageBefore = pet.stage;
  offlineInfo.phaseBefore = phase;

  const float beforeFaim = pet.faim;
  const float beforeSoif = pet.soif;
  const float beforeHygiene = pet.hygiene;
  const float beforeHumeur = pet.humeur;
  const float beforeFatigue = pet.fatigue;
  const float beforeAmour = pet.amour;
  const float beforeSante = pet.sante;
  const uint32_t beforeCoin = pet.lungmenCoin;

  if (!berriesLeftAvailable) {
    berriesLeftAvailable = true;
    berriesRespawnAt = 0;
    offlineInfo.supplyRespawned = true;
  }
  if (!puddleVisible) {
    puddleVisible = true;
    puddleRespawnAt = 0;
    offlineInfo.waterRespawned = true;
  }

  task.active = false;
  task.kind = TASK_NONE;
  task.ph = PH_GO;
  appMode = MODE_PET;
  mg.active = false;
  mg.kind = TASK_NONE;
  activityVisible = false;
  activityText[0] = 0;
  showMsg = false;
  msgText[0] = 0;

  TriState restoreState = savedWasSleeping ? ST_SLEEP : ST_SIT;
  enterState(restoreState, now);

  offlineSettlementInProgress = true;
  for (uint32_t i = 0; i < offlineInfo.appliedMin; ++i) {
    applyOfflineMinuteTick(now, offlineInfo.sleepingMode);
    if (!pet.vivant || phase == PHASE_TOMB) {
      offlineInfo.petDied = true;
      break;
    }
    if (phase != PHASE_ALIVE) break;
  }
  offlineSettlementInProgress = false;

  offlineInfo.stageAfter = pet.stage;
  offlineInfo.phaseAfter = phase;
  offlineInfo.stageChanged = (offlineInfo.stageAfter != offlineInfo.stageBefore);
  offlineInfo.phaseChanged = (offlineInfo.phaseAfter != offlineInfo.phaseBefore);
  offlineInfo.endedCritical = pet.vivant && (healthCriticalCount() > 0);
  offlineInfo.deltaFaim = roundDelta(pet.faim, beforeFaim);
  offlineInfo.deltaSoif = roundDelta(pet.soif, beforeSoif);
  offlineInfo.deltaHygiene = roundDelta(pet.hygiene, beforeHygiene);
  offlineInfo.deltaHumeur = roundDelta(pet.humeur, beforeHumeur);
  offlineInfo.deltaFatigue = roundDelta(pet.fatigue, beforeFatigue);
  offlineInfo.deltaAmour = roundDelta(pet.amour, beforeAmour);
  offlineInfo.deltaSante = roundDelta(pet.sante, beforeSante);
  offlineInfo.deltaCoin = (int32_t)(pet.lungmenCoin - beforeCoin);

  if (pet.vivant) {
    if (phase == PHASE_ALIVE || phase == PHASE_RESTREADY) enterState(restoreState, now);
  }

  offlineInfo.applied = true;
  buildOfflineNotice();

  if (saveReady) saveNow(now, "boot_offline");
}

static void applyDailyEventNoticeEffects(const DailyEventNotice& notice, uint32_t now) {
  if (!pet.vivant) return;
  auto add = [](float &v, int dv){ v = clamp01f(v + (float)dv); };
  add(pet.faim, notice.deltaFaim);
  add(pet.soif, notice.deltaSoif);
  add(pet.fatigue, notice.deltaFatigue);
  add(pet.hygiene, notice.deltaHygiene);
  add(pet.humeur, notice.deltaHumeur);
  add(pet.amour, notice.deltaAmour);
  add(pet.sante, notice.deltaSante);
  updateHealthTick(now);
  uiSpriteDirty = true;
  uiForceBands = true;
}

static int chooseDailyEventIndex(const bool used[]) {
  if (!dailyEventTableReady || dailyEventCount == 0) return -1;
  uint32_t totalWeight = 0;
  for (uint8_t i = 0; i < dailyEventCount; ++i) {
    if (!dailyEventDefs[i].enabled || dailyEventDefs[i].weight == 0) continue;
    if (used && used[i]) continue;
    totalWeight += dailyEventDefs[i].weight;
  }
  if (totalWeight == 0) return -1;

  uint32_t roll = (uint32_t)random(0, (long)totalWeight);
  uint32_t acc = 0;
  for (uint8_t i = 0; i < dailyEventCount; ++i) {
    if (!dailyEventDefs[i].enabled || dailyEventDefs[i].weight == 0) continue;
    if (used && used[i]) continue;
    acc += dailyEventDefs[i].weight;
    if (roll < acc) return (int)i;
  }
  return -1;
}

static void queueDailyEventsForDay(uint32_t dayIndex, bool offlineFlag, uint32_t now) {
  if (!dailyEventTableReady || dailyEventCount == 0) return;
  bool used[DAILY_EVENT_MAX] = {0};
  uint8_t count = (uint8_t)random(1, 4);
  if (count > dailyEventCount) count = dailyEventCount;

  for (uint8_t n = 0; n < count; ++n) {
    int picked = chooseDailyEventIndex(used);
    if (picked < 0) break;
    used[picked] = true;

    const DailyEventDef& def = dailyEventDefs[picked];
    DailyEventNotice notice;
    memset(&notice, 0, sizeof(notice));
    notice.valid = true;
    notice.offline = offlineFlag;
    notice.dayIndex = dayIndex;
    notice.art = (def.art == AUTO_ART_AUTO) ? AUTO_ART_ASSIE : def.art;
    strncpy(notice.eventName, def.eventName, sizeof(notice.eventName) - 1);
    strncpy(notice.summary, def.summary, sizeof(notice.summary) - 1);
    notice.deltaFaim = def.deltaFaim;
    notice.deltaSoif = def.deltaSoif;
    notice.deltaFatigue = def.deltaFatigue;
    notice.deltaHygiene = def.deltaHygiene;
    notice.deltaHumeur = def.deltaHumeur;
    notice.deltaAmour = def.deltaAmour;
    notice.deltaSante = def.deltaSante;

    applyDailyEventNoticeEffects(notice, now);
    enqueueDailyEventNotice(notice);
    if (!pet.vivant) break;
  }
}

static void maybeOpenPendingDailyEventModal() {
  if (isModalMode() || isMiniGameMode()) return;
  if (task.active) return;
  if (!hasPendingDailyEventNotice()) return;
  openDailyEventModal();
}

static void processDailyEvents(uint32_t now, bool fromBoot) {
  uint32_t unixTime = 0;
  if (!readCurrentUnixTime(unixTime)) return;

  uint32_t currentDay = unixTime / 86400UL;
  if (currentDay == 0) return;

  if (lastDailyEventDay == 0) {
    lastDailyEventDay = currentDay;
    if (saveReady) saveNow(now, "event_seed");
    return;
  }
  if (currentDay <= lastDailyEventDay) return;
  if (phase != PHASE_ALIVE) {
    lastDailyEventDay = currentDay;
    if (saveReady) saveNow(now, "event_skip");
    return;
  }

  for (uint32_t day = lastDailyEventDay + 1; day <= currentDay; ++day) {
    queueDailyEventsForDay(day, fromBoot, now);
    if (!pet.vivant) break;
  }
  lastDailyEventDay = currentDay;
  if (saveReady) saveNow(now, fromBoot ? "event_boot" : "event_live");
}

// ================== Idle autonome (config table driven) ==================
static int chooseAutoBehaviorIndex() {
  if (!autoBehaviorTableReady || autoBehaviorCount == 0) return -1;

  uint32_t totalWeight = 0;
  for (uint8_t i = 0; i < autoBehaviorCount; ++i) {
    if (!autoBehaviorDefs[i].enabled || autoBehaviorDefs[i].weight == 0) continue;
    totalWeight += autoBehaviorDefs[i].weight;
  }
  if (totalWeight == 0) return -1;

  uint32_t roll = (uint32_t)random(0, (long)totalWeight);
  uint32_t acc = 0;
  for (uint8_t i = 0; i < autoBehaviorCount; ++i) {
    if (!autoBehaviorDefs[i].enabled || autoBehaviorDefs[i].weight == 0) continue;
    acc += autoBehaviorDefs[i].weight;
    if (roll < acc) return (int)i;
  }
  return (int)(autoBehaviorCount - 1);
}

static void startConfiguredAutoBehavior(uint8_t index, uint32_t now) {
  if (index >= autoBehaviorCount) return;

  const AutoBehaviorDef& def = autoBehaviorDefs[index];
  clearAutoBehavior();

  autoBehavior.active = true;
  autoBehavior.index = (int8_t)index;
  autoBehavior.moveMode = def.moveMode;
  autoBehavior.art = (def.art == AUTO_ART_AUTO) ? defaultAutoArtForMove(def.moveMode) : def.art;
  strncpy(autoBehavior.name, def.behavior, sizeof(autoBehavior.name) - 1);
  chooseBehaviorBubbleText(def.bubble, autoBehavior.bubble, sizeof(autoBehavior.bubble));
  if (autoBehavior.bubble[0] == 0) {
    strncpy(autoBehavior.bubble, autoBehavior.name, sizeof(autoBehavior.bubble) - 1);
  }

  uint32_t minMs = def.minMs;
  uint32_t maxMs = max((uint32_t)def.maxMs, minMs);
  uint32_t span = maxMs - minMs;
  uint32_t dur = minMs + (span > 0 ? (uint32_t)random(0, (long)span + 1L) : 0UL);
  autoBehavior.until = now + dur;
  autoBehavior.bubbleUntil = now + min((uint32_t)dur, (uint32_t)AUTO_BEHAVIOR_BUBBLE_MS);

  if (def.moveMode == AUTO_MOVE_WALK) {
    float minX = worldMin + 10.0f;
    float maxX = worldMax - (float)triW(pet.stage) - 10.0f;
    if (maxX <= minX) maxX = minX + 1.0f;
    idleTargetX = (float)random((long)minX, (long)maxX);
    idleWalking = true;
    movingRight = (idleTargetX >= worldX);
    enterState(ST_WALK, now);
  } else {
    idleWalking = false;
    if (autoBehavior.art == AUTO_ART_CLIGNE) enterState(ST_BLINK, now);
    else enterState(ST_SIT, now);
  }

  nextIdleDecisionAt = autoBehavior.until + (uint32_t)random((long)AUTO_BEHAVIOR_MIN_GAP_MS, (long)AUTO_BEHAVIOR_MAX_GAP_MS + 1L);
}

static void startFallbackIdleBehavior(uint32_t now) {
  uint32_t r = (uint32_t)random(0, 100);

  clearAutoBehavior();
  if (r < 80) {
    float minX = worldMin + 10.0f;
    float maxX = worldMax - (float)triW(pet.stage) - 10.0f;
    idleTargetX = (float)random((long)minX, (long)maxX);
    idleWalking = true;
    idleUntil = now + (uint32_t)random(3000, 8500);
    enterState(ST_WALK, now);
  } else if (r < 90) {
    idleWalking = false;
    idleUntil = now + (uint32_t)random(900, 3000);
    enterState(ST_SIT, now);
  } else {
    idleWalking = false;
    idleUntil = now + (uint32_t)random(900, 2000);
    enterState(ST_BLINK, now);
  }
  nextIdleDecisionAt = now + (uint32_t)random(2000, 5000);
}

static void idleDecide(uint32_t now) {
  if (phase != PHASE_ALIVE) return;
  if (task.active) return;
  if (state == ST_SLEEP) return;
  if (appMode != MODE_PET) return;

  int pick = chooseAutoBehaviorIndex();
  if (pick >= 0) {
    startConfiguredAutoBehavior((uint8_t)pick, now);
  } else {
    startFallbackIdleBehavior(now);
  }
}

static void idleUpdate(uint32_t now) {
  if (phase != PHASE_ALIVE) return;
  if (task.active) return;
  if (state == ST_SLEEP) return;
  if (appMode != MODE_PET) return;

  if (nextIdleDecisionAt == 0 || (int32_t)(now - nextIdleDecisionAt) >= 0) {
    idleDecide(now);
  }

  if (autoBehavior.active && idleUntil == 0 && (int32_t)(now - autoBehavior.until) >= 0) {
    clearAutoBehavior();
    enterState(ST_SIT, now);
  } else if (!autoBehavior.active && idleUntil != 0 && (int32_t)(now - idleUntil) >= 0) {
    idleUntil = 0;
    enterState(ST_SIT, now);
  }

  if (idleWalking && state == ST_WALK) {
    float goal = idleTargetX;
    if (fabsf(goal - worldX) < 1.2f) {
      worldX = goal;
      idleWalking = false;
      enterState(ST_SIT, now);
      if (autoBehavior.active && autoBehavior.until < now + 500UL) autoBehavior.until = now + 500UL;
      if (!autoBehavior.active) idleUntil = now + (uint32_t)random(400, 1200);
      return;
    }
    movingRight = (goal >= worldX);
    float sp = moveSpeedPxPerFrame();
    float dir = movingRight ? 1.0f : -1.0f;
    worldX += dir * sp;
    worldX = clampf(worldX, worldMin, worldMax);
  }
}
