#pragma once

// Phase 1 refactor:
// Extract persistence, RTC, and storage initialization helpers out of the main .ino
// while preserving a single translation unit and the existing runtime behavior.

bool saveReady = false;      // true si le FS de sauvegarde est prêt
#if USE_LITTLEFS_SAVE
  fs::FS& saveFS = LittleFS;  // sauvegardes sur Flash interne
#else
  fs::FS& saveFS = SD;        // sauvegardes sur carte SD
#endif

static const char* SAVE_A  = "/saveA.json";
static const char* SAVE_B  = "/saveB.json";
static const char* TMP_A   = "/saveA.tmp";
static const char* TMP_B   = "/saveB.tmp";

static const uint32_t SAVE_EVERY_MS = 60000UL;
static const uint32_t OFFLINE_SETTLE_CAP_MIN = 24UL * 60UL;
static const uint32_t OFFLINE_REPORT_MS = 3200UL;

static uint32_t saveSeq = 0;
static uint32_t lastSaveAt = 0;
static bool nextSlotIsA = true;
static bool savedTimeValid = false;
static uint32_t savedLastUnixTime = 0;
static bool savedWasSleeping = false;
static char bootOfflineNotice[96] = {0};
static char bootStorageNotice[96] = {0};
static uint16_t bootStorageNoticeColor = TFT_WHITE;
static char bootAssetNotice[96] = {0};
static uint16_t bootAssetNoticeColor = TFT_RED;

static inline int iClamp(int v, int lo, int hi){ if(v<lo) return lo; if(v>hi) return hi; return v; }
static inline int fToI100(float f){ return iClamp((int)lroundf(f), 0, 100); }

struct OfflineSettlementInfo {
  bool checked = false;
  bool applied = false;
  bool skippedNoClock = false;
  bool skippedInvalidRtc = false;
  bool skippedNoSavedTime = false;
  bool skippedInvalidDelta = false;
  bool capped = false;
  uint32_t elapsedSec = 0;
  uint32_t appliedMin = 0;
  bool petDied = false;
  bool sleepingMode = false;
  bool supplyRespawned = false;
  bool waterRespawned = false;
  bool stageChanged = false;
  bool phaseChanged = false;
  bool endedCritical = false;
  int16_t deltaFaim = 0;
  int16_t deltaSoif = 0;
  int16_t deltaHygiene = 0;
  int16_t deltaHumeur = 0;
  int16_t deltaFatigue = 0;
  int16_t deltaAmour = 0;
  int16_t deltaSante = 0;
  int32_t deltaCoin = 0;
  AgeStage stageBefore = AGE_JUNIOR;
  AgeStage stageAfter = AGE_JUNIOR;
  GamePhase phaseBefore = PHASE_EGG;
  GamePhase phaseAfter = PHASE_EGG;
};
static OfflineSettlementInfo offlineInfo;

static constexpr uint8_t RTC_DS3231_ADDR = 0x68;
static constexpr uint32_t RTC_I2C_HZ = 400000UL;
static bool rtcInitTried = false;
static bool rtcWireReady = false;
static bool rtcPresent = false;
static bool rtcTimeValid = false;

static inline uint8_t bcdToDec(uint8_t v) {
  return (uint8_t)(((v >> 4) * 10U) + (v & 0x0F));
}

static inline bool isLeapYear(int year) {
  return ((year % 4) == 0 && (year % 100) != 0) || ((year % 400) == 0);
}

static inline uint8_t daysInMonth(int year, int month) {
  static const uint8_t kDays[12] = {31,28,31,30,31,30,31,31,30,31,30,31};
  if (month < 1 || month > 12) return 0;
  if (month == 2 && isLeapYear(year)) return 29;
  return kDays[month - 1];
}

static int32_t daysFromCivil(int year, unsigned month, unsigned day) {
  year -= (month <= 2) ? 1 : 0;
  const int era = (year >= 0 ? year : year - 399) / 400;
  const unsigned yoe = (unsigned)(year - era * 400);
  const int monthPrime = (int)month + (month > 2 ? -3 : 9);
  const unsigned doy = (153U * (unsigned)monthPrime + 2U) / 5U + day - 1U;
  const unsigned doe = yoe * 365U + yoe / 4U - yoe / 100U + doy;
  return era * 146097 + (int32_t)doe - 719468;
}

static bool rtcEnsureWire() {
  if (rtcWireReady) return true;

#if DISPLAY_PROFILE == DISPLAY_PROFILE_2432S022
  Wire.setClock(RTC_I2C_HZ);
  rtcWireReady = true;
#else
  Wire.begin(RTC_SDA, RTC_SCL);
  Wire.setClock(RTC_I2C_HZ);
  rtcWireReady = true;
#endif

  return true;
}

static bool rtcReadRegisters(uint8_t reg, uint8_t* dst, size_t len) {
  if (!rtcEnsureWire()) return false;

  Wire.beginTransmission(RTC_DS3231_ADDR);
  Wire.write(reg);
  if (Wire.endTransmission(false) != 0) return false;

  size_t got = Wire.requestFrom((int)RTC_DS3231_ADDR, (int)len);
  if (got != len) {
    while (Wire.available()) Wire.read();
    return false;
  }

  for (size_t i = 0; i < len; ++i) {
    dst[i] = (uint8_t)Wire.read();
  }
  return true;
}

static bool rtcProbeDevice() {
  if (!rtcEnsureWire()) return false;
  Wire.beginTransmission(RTC_DS3231_ADDR);
  return Wire.endTransmission() == 0;
}

static bool rtcReadUnixTimeFromChip(uint32_t& outUnixTime) {
  outUnixTime = 0;

  uint8_t status = 0;
  if (!rtcReadRegisters(0x0F, &status, 1)) return false;
  if (status & 0x80U) return false;

  uint8_t raw[7] = {0};
  if (!rtcReadRegisters(0x00, raw, sizeof(raw))) return false;

  int second = bcdToDec(raw[0] & 0x7F);
  int minute = bcdToDec(raw[1] & 0x7F);

  int hour = 0;
  if (raw[2] & 0x40U) {
    hour = bcdToDec(raw[2] & 0x1F);
    bool pm = (raw[2] & 0x20U) != 0;
    if (hour == 12) hour = 0;
    if (pm) hour += 12;
  } else {
    hour = bcdToDec(raw[2] & 0x3F);
  }

  int day = bcdToDec(raw[4] & 0x3F);
  int month = bcdToDec(raw[5] & 0x1F);
  int year = 2000 + bcdToDec(raw[6]);

  if (second > 59 || minute > 59 || hour > 23) return false;
  if (month < 1 || month > 12) return false;
  if (day < 1 || day > (int)daysInMonth(year, month)) return false;

  int32_t days = daysFromCivil(year, (unsigned)month, (unsigned)day);
  if (days < 0) return false;

  uint32_t seconds = (uint32_t)second + (uint32_t)minute * 60UL + (uint32_t)hour * 3600UL;
  outUnixTime = (uint32_t)days * 86400UL + seconds;
  return true;
}

static bool timeSourceInit() {
  if (rtcInitTried) return rtcPresent && rtcTimeValid;
  rtcInitTried = true;

  rtcPresent = false;
  for (uint8_t attempt = 0; attempt < 8 && !rtcPresent; ++attempt) {
    rtcPresent = rtcProbeDevice();
    if (!rtcPresent) delay(50);
  }
  if (!rtcPresent) {
    Serial.printf("[TIME] DS3231 not found on I2C SDA=%d SCL=%d\n", RTC_SDA, RTC_SCL);
    rtcTimeValid = false;
    return false;
  }

  uint32_t unixTime = 0;
  rtcTimeValid = rtcReadUnixTimeFromChip(unixTime);
  if (rtcTimeValid) {
    Serial.printf("[TIME] DS3231 detected, unix=%lu\n", (unsigned long)unixTime);
  } else {
    Serial.println("[TIME] DS3231 detected but time is invalid; set RTC first");
  }
  return rtcTimeValid;
}

static bool readCurrentUnixTime(uint32_t& outUnixTime) {
  if (!rtcInitTried) timeSourceInit();
  if (!rtcPresent) {
    outUnixTime = 0;
    return false;
  }

  rtcTimeValid = rtcReadUnixTimeFromChip(outUnixTime);
  return rtcTimeValid;
}

static inline int16_t roundDelta(float after, float before) {
  return (int16_t)lroundf(after - before);
}

static void appendNoticeTag(char* dst, size_t dstSize, const char* tag) {
  size_t len = strlen(dst);
  if (len + 1 >= dstSize) return;
  strncat(dst, tag, dstSize - len - 1);
}

static inline uint8_t decToBcd(uint8_t v) {
  return (uint8_t)(((v / 10U) << 4) | (v % 10U));
}

static uint8_t rtcWeekdayFromDate(int year, int month, int day) {
  int32_t days = daysFromCivil(year, (unsigned)month, (unsigned)day);
  if (days < 0) return 1;
  return (uint8_t)(((days + 4) % 7 + 7) % 7 + 1);
}

static int buildMonthFromName(const char* mon) {
  static const char* kMonths[12] = {"Jan","Feb","Mar","Apr","May","Jun","Jul","Aug","Sep","Oct","Nov","Dec"};
  for (int i = 0; i < 12; ++i) {
    if (strncmp(mon, kMonths[i], 3) == 0) return i + 1;
  }
  return 1;
}

static void resetRtcDraftFromBuildTime() {
  const char* d = __DATE__;
  const char* t = __TIME__;
  rtcDraft.year = atoi(d + 7);
  rtcDraft.month = buildMonthFromName(d);
  rtcDraft.day = atoi(d + 4);
  rtcDraft.hour = ((t[0] - '0') * 10) + (t[1] - '0');
  rtcDraft.minute = ((t[3] - '0') * 10) + (t[4] - '0');
  rtcDraft.field = 0;
}

static void rtcClampDraft() {
  rtcDraft.year = clampi(rtcDraft.year, 2024, 2099);
  rtcDraft.month = clampi(rtcDraft.month, 1, 12);
  rtcDraft.day = clampi(rtcDraft.day, 1, (int)daysInMonth(rtcDraft.year, rtcDraft.month));
  rtcDraft.hour = clampi(rtcDraft.hour, 0, 23);
  rtcDraft.minute = clampi(rtcDraft.minute, 0, 59);
}

static bool rtcWriteRegisters(uint8_t reg, const uint8_t* src, size_t len) {
  if (!rtcEnsureWire()) return false;
  Wire.beginTransmission(RTC_DS3231_ADDR);
  Wire.write(reg);
  for (size_t i = 0; i < len; ++i) Wire.write(src[i]);
  return Wire.endTransmission() == 0;
}

static bool rtcWriteDraftToChip() {
  rtcClampDraft();
  uint8_t raw[7];
  raw[0] = decToBcd(0);
  raw[1] = decToBcd((uint8_t)rtcDraft.minute);
  raw[2] = decToBcd((uint8_t)rtcDraft.hour);
  raw[3] = decToBcd(rtcWeekdayFromDate(rtcDraft.year, rtcDraft.month, rtcDraft.day));
  raw[4] = decToBcd((uint8_t)rtcDraft.day);
  raw[5] = decToBcd((uint8_t)rtcDraft.month);
  raw[6] = decToBcd((uint8_t)(rtcDraft.year - 2000));
  if (!rtcWriteRegisters(0x00, raw, sizeof(raw))) return false;

  uint8_t status = 0;
  if (!rtcReadRegisters(0x0F, &status, 1)) return false;
  status &= (uint8_t)~0x80U;
  if (!rtcWriteRegisters(0x0F, &status, 1)) return false;
  rtcPresent = true;
  rtcTimeValid = true;
  rtcInitTried = true;
  return true;
}

static inline bool isMiniGameMode() {
  return appMode == MODE_MG_WASH || appMode == MODE_MG_PLAY;
}

static inline bool isModalMode() {
  return appMode == MODE_MODAL_REPORT || appMode == MODE_RTC_PROMPT || appMode == MODE_RTC_SET || appMode == MODE_MODAL_EVENT || appMode == MODE_SCENE_SELECT;
}

static inline bool runtimeSleepingFlag() {
  return (state == ST_SLEEP) || (task.active && task.kind == TASK_SLEEP);
}

static bool sdInit() {
  sdSPI.begin(SD_SCK, SD_MISO, SD_MOSI, SD_CS);
  sdReady = SD.begin(SD_CS, sdSPI, 20000000);
  if (!sdReady) {
    Serial.println("[SD] init FAIL (pas de carte SD - normal si non utilisee)");
  } else {
    Serial.println("[SD] init OK");
  }
  return sdReady;
}

#if USE_LITTLEFS_SAVE
static bool lfsInit() {
  if (!LittleFS.begin(true)) {
    Serial.println("[LittleFS] init FAIL");
    return false;
  }
  Serial.println("[LittleFS] init OK");
  return true;
}
#endif

static void storageInit() {
#if USE_LITTLEFS_SAVE
  saveReady = lfsInit();
  sdInit();
  Serial.printf("[STORAGE] Save=%s (LittleFS), SD=%s\n",
                saveReady ? "OK" : "FAIL",
                sdReady ? "OK" : "non connectee");
  snprintf(bootStorageNotice, sizeof(bootStorageNotice),
           saveReady ? "LittleFS check: save ready" : "LittleFS check: init failed");
  bootStorageNoticeColor = saveReady ? TFT_GREEN : TFT_RED;
#else
  sdInit();
  saveReady = sdReady;
  Serial.printf("[STORAGE] Save=%s (SD)\n", saveReady ? "OK" : "FAIL");
  snprintf(bootStorageNotice, sizeof(bootStorageNotice),
           saveReady ? "SD check: save ready" : "SD check: no card, save off");
  bootStorageNoticeColor = saveReady ? TFT_GREEN : TFT_RED;
#endif
  bootStorageNotice[sizeof(bootStorageNotice) - 1] = 0;
}

static bool readJsonFile(const char* path, StaticJsonDocument<4096>& doc, uint32_t& outSeq) {
  outSeq = 0;
  if (!saveReady) return false;
  if (!saveFS.exists(path)) return false;

  File f = saveFS.open(path, FILE_READ);
  if (!f) return false;

  DeserializationError err = deserializeJson(doc, f);
  f.close();
  if (err) return false;

  int ver = doc["ver"] | 0;
  if (ver < 1 || ver > 2) return false;

  outSeq = doc["seq"] | 0UL;
  if (!doc.containsKey("phase")) return false;
  if (!doc.containsKey("stage")) return false;
  if (!doc.containsKey("ageMin")) return false;
  if (!doc.containsKey("pet")) return false;

  return true;
}

static void applyLoadedToRuntime(uint32_t now) {
  task.active = false;
  task.kind = TASK_NONE;
  task.ph = PH_GO;
  task.doUntil = 0;
  task.startedAt = now;
  task.plannedTotal = 0;
  task.doMs = 0;

  for (int i = 0; i < (int)UI_COUNT; i++) cdUntil[i] = 0;

  worldX = homeX;
  camX   = worldX - (float)(SW / 2);
  camX = clampf(camX, 0.0f, worldW - (float)SW);

  poopVisible = false; poopUntil = 0;
  berriesLeftAvailable = true; berriesRespawnAt = 0;
  puddleVisible = true; puddleRespawnAt = 0;

  activityVisible = false;
  activityText[0] = 0;
  showMsg = false;
  uiSel = 0;

  animIdx = 0;
  nextAnimTick = 0;
  enterState(ST_SIT, now);

  appMode = MODE_PET;
  mg.active = false;
  mg.kind = TASK_NONE;
  clearAutoBehavior();

  uiSpriteDirty = true;
  uiForceBands  = true;
}

static bool loadLatestSave(uint32_t now) {
  if (!saveReady) return false;

  static StaticJsonDocument<4096> dA, dB;
  dA.clear();
  dB.clear();
  uint32_t sA=0, sB=0;
  bool okA = readJsonFile(SAVE_A, dA, sA);
  bool okB = readJsonFile(SAVE_B, dB, sB);

  if (!okA && !okB) return false;

  StaticJsonDocument<4096>* bestDoc = nullptr;
  bool bestIsA = true;

  if (okA && (!okB || sA >= sB)) { bestDoc = &dA; bestIsA = true; }
  else                           { bestDoc = &dB; bestIsA = false; }

  auto& doc = *bestDoc;

  saveSeq = doc["seq"] | 0UL;

  int p  = doc["phase"] | (int)PHASE_EGG;
  int st = doc["stage"] | (int)AGE_JUNIOR;

  if (p == (int)PHASE_HATCHING) p = (int)PHASE_EGG;

  phase = (GamePhase)iClamp(p, (int)PHASE_EGG, (int)PHASE_TOMB);
  pet.stage = (AgeStage)iClamp(st, (int)AGE_JUNIOR, (int)AGE_SENIOR);

  pet.ageMin = doc["ageMin"] | 0UL;
  pet.evolveProgressMin = doc["evolveProgressMin"] | 0UL;
  pet.lungmenCoin = doc["lungmenCoin"] | 0UL;
  currentScene = (SceneId)clampi(doc["sceneId"] | (int)SCENE_DORM, 0, (int)SCENE_COUNT - 1);
  applySceneConfig(currentScene, false);
  pet.vivant = doc["vivant"] | true;
  savedTimeValid = doc["timeValid"] | false;
  savedLastUnixTime = doc["lastUnixTime"] | 0UL;
  savedWasSleeping = doc["lastSleeping"] | false;
  lastDailyEventDay = doc["dailyLastDay"] | 0UL;
  clearDailyEventQueue();
  JsonArray pendingEvents = doc["dailyPending"].as<JsonArray>();
  if (!pendingEvents.isNull()) {
    for (JsonObject item : pendingEvents) {
      if (dailyEventQueueCount >= DAILY_EVENT_QUEUE_MAX) break;
      DailyEventNotice notice;
      memset(&notice, 0, sizeof(notice));
      notice.valid = true;
      notice.offline = item["offline"] | false;
      notice.dayIndex = item["day"] | 0UL;
      notice.art = (AutoBehaviorArt)clampi(item["art"] | (int)AUTO_ART_ASSIE, (int)AUTO_ART_AUTO, (int)AUTO_ART_MANGE);
      const char* nm = item["name"] | "";
      const char* sm = item["summary"] | "";
      strncpy(notice.eventName, nm, sizeof(notice.eventName) - 1);
      strncpy(notice.summary, sm, sizeof(notice.summary) - 1);
      notice.deltaFaim = (int8_t)clampi(item["dfaim"] | 0, -100, 100);
      notice.deltaSoif = (int8_t)clampi(item["dsoif"] | 0, -100, 100);
      notice.deltaFatigue = (int8_t)clampi(item["dfatigue"] | 0, -100, 100);
      notice.deltaHygiene = (int8_t)clampi(item["dhygiene"] | 0, -100, 100);
      notice.deltaHumeur = (int8_t)clampi(item["dhumeur"] | 0, -100, 100);
      notice.deltaAmour = (int8_t)clampi(item["damour"] | 0, -100, 100);
      notice.deltaSante = (int8_t)clampi(item["dsante"] | 0, -100, 100);
      enqueueDailyEventNotice(notice);
    }
  }

  const char* nm = doc["name"] | "???";
  strncpy(petName, nm, sizeof(petName)-1);
  petName[sizeof(petName)-1] = 0;

  JsonObject ps = doc["pet"].as<JsonObject>();
  pet.faim    = (float)iClamp(ps["faim"]    | 60, 0, 100);
  pet.soif    = (float)iClamp(ps["soif"]    | 60, 0, 100);
  pet.hygiene = (float)iClamp(ps["hygiene"] | 80, 0, 100);
  pet.humeur  = (float)iClamp(ps["humeur"]  | 60, 0, 100);
  pet.fatigue = (float)iClamp(ps["fatigue"] | 100,0, 100);
  pet.amour   = (float)iClamp(ps["amour"]   | 60, 0, 100);
  pet.sante   = (float)iClamp(ps["sante"]   | 80, 0, 100);

#if ENABLE_AUDIO
  audioMode = (AudioMode)iClamp(doc["audioMode"] | (int)AUDIO_TOTAL, (int)AUDIO_OFF, (int)AUDIO_TOTAL);
  audioVolumePercent = (uint8_t)iClamp(doc["audioVol"] | (int)audioVolumePercent, 1, 100);
#else
  audioMode = AUDIO_OFF;
#endif

  if (!pet.vivant || pet.sante <= 0.0f) {
    pet.vivant = false;
    phase = PHASE_TOMB;
  }

  nextSlotIsA = !bestIsA;

  applyLoadedToRuntime(now);
  Serial.printf("[SAVE] Loaded seq=%lu from %s\n",
                (unsigned long)saveSeq,
                bestIsA ? "A" : "B");
  return true;
}

static bool writeSlotFile(const char* tmpPath, const char* finalPath, const char* why, bool timeValid, uint32_t unixTime) {
  static StaticJsonDocument<4096> doc;
  doc.clear();
  doc["ver"] = 2;
  doc["seq"] = saveSeq;
  doc["why"] = why;

  doc["phase"] = (int)phase;
  doc["stage"] = (int)pet.stage;
  doc["sceneId"] = (int)currentScene;
  doc["ageMin"] = (uint32_t)pet.ageMin;
  doc["evolveProgressMin"] = (uint32_t)pet.evolveProgressMin;
  doc["lungmenCoin"] = (uint32_t)pet.lungmenCoin;
  doc["vivant"] = pet.vivant;
  doc["name"] = petName;
  doc["timeValid"] = timeValid;
  doc["lastUnixTime"] = timeValid ? unixTime : 0UL;
  doc["lastSleeping"] = runtimeSleepingFlag();
  doc["dailyLastDay"] = lastDailyEventDay;

#if ENABLE_AUDIO
  doc["audioMode"] = (int)audioMode;
  doc["audioVol"] = (int)audioVolumePercent;
#endif

  JsonObject ps = doc.createNestedObject("pet");
  ps["faim"]    = fToI100(pet.faim);
  ps["soif"]    = fToI100(pet.soif);
  ps["hygiene"] = fToI100(pet.hygiene);
  ps["humeur"]  = fToI100(pet.humeur);
  ps["fatigue"] = fToI100(pet.fatigue);
  ps["amour"]   = fToI100(pet.amour);
  ps["sante"]   = fToI100(pet.sante);

  JsonArray pendingEvents = doc.createNestedArray("dailyPending");
  for (uint8_t i = 0; i < dailyEventQueueCount; ++i) {
    const DailyEventNotice& notice = dailyEventQueue[i];
    if (!notice.valid) continue;
    JsonObject item = pendingEvents.createNestedObject();
    item["offline"] = notice.offline;
    item["day"] = notice.dayIndex;
    item["art"] = (int)notice.art;
    item["name"] = notice.eventName;
    item["summary"] = notice.summary;
    item["dfaim"] = notice.deltaFaim;
    item["dsoif"] = notice.deltaSoif;
    item["dfatigue"] = notice.deltaFatigue;
    item["dhygiene"] = notice.deltaHygiene;
    item["dhumeur"] = notice.deltaHumeur;
    item["damour"] = notice.deltaAmour;
    item["dsante"] = notice.deltaSante;
  }

  if (saveFS.exists(tmpPath)) saveFS.remove(tmpPath);
  File f = saveFS.open(tmpPath, FILE_WRITE);
  if (!f) return false;

  if (serializeJson(doc, f) == 0) { f.close(); return false; }
  f.flush();
  f.close();

  if (saveFS.exists(finalPath)) saveFS.remove(finalPath);
  if (!saveFS.rename(tmpPath, finalPath)) {
    saveFS.remove(tmpPath);
    return false;
  }
  return true;
}

static bool saveNow(uint32_t now, const char* why) {
  if (!saveReady) return false;
  if (offlineSettlementInProgress) return false;

  uint32_t unixTime = 0;
  bool timeValid = readCurrentUnixTime(unixTime);
  savedTimeValid = timeValid;
  savedLastUnixTime = timeValid ? unixTime : 0UL;
  savedWasSleeping = runtimeSleepingFlag();

  saveSeq++;
  const bool useA = nextSlotIsA;
  nextSlotIsA = !nextSlotIsA;

  const char* tmp  = useA ? TMP_A  : TMP_B;
  const char* fin  = useA ? SAVE_A : SAVE_B;

  bool ok = writeSlotFile(tmp, fin, why, timeValid, unixTime);
  if (ok) {
    lastSaveAt = now;
  } else {
    Serial.println("[SAVE] FAIL write");
  }
  return ok;
}

static inline void saveMaybeEachMinute(uint32_t now) {
  if (!saveReady) return;
  if (lastSaveAt == 0) lastSaveAt = now;
  if ((uint32_t)(now - lastSaveAt) >= SAVE_EVERY_MS) {
    saveNow(now, "minute");
  }
}
