#pragma once

// Pet task flow, simulation ticks, health/evolution, and reset helpers.
// Extracted from JurassicLifeCN.ino during the refactor.

// ================== enterState / timing ==================
static inline uint16_t frameMsForState(TriState st) {
  switch (st) {
    case ST_BLINK: return 90;
    case ST_SLEEP: return 180;
    case ST_SIT:   return 140;
    case ST_EAT:   return 120;
    case ST_DEAD:  return 220;
    default:       return 120;
  }
}
static void enterState(TriState st, uint32_t now) {
  if (state == st) return;
  state = st;
  animIdx = 0;
  nextAnimTick = now + frameMsForState(st);
}

// ================== Effects ==================
static void applyTaskEffects(TaskKind k, uint32_t now) {
  if (!pet.vivant) return;

  float gm = gainMulForStage(pet.stage);
  auto add = [&](float &v, float dv){ v = clamp01f(v + dv * gm); };

  switch (k) {
    case TASK_EAT:
      add(pet.faim,    EAT_HUNGER);
      add(pet.fatigue, EAT_FATIGUE);
      add(pet.soif,    EAT_THIRST);
      add(pet.humeur,  EAT_MOOD);
      break;

    case TASK_DRINK:
      add(pet.soif,    DRINK_THIRST);
      add(pet.fatigue, DRINK_FATIGUE);
      add(pet.humeur,  DRINK_MOOD);
      break;

    case TASK_WASH:
      add(pet.hygiene, WASH_HYGIENE);
      add(pet.humeur,  WASH_MOOD);
      add(pet.fatigue, WASH_FATIGUE);
      break;

    case TASK_PLAY:
      add(pet.humeur,  PLAY_MOOD);
      add(pet.fatigue, PLAY_FATIGUE);
      add(pet.faim,    PLAY_HUNGER);
      add(pet.soif,    PLAY_THIRST);
      add(pet.amour,   PLAY_LOVE);
      break;

    case TASK_POOP:
      add(pet.hygiene, CLEAN_HYGIENE);
      add(pet.humeur,  CLEAN_MOOD);
      add(pet.amour,   CLEAN_LOVE);
      add(pet.fatigue, CLEAN_FATIGUE);
      break;

    case TASK_HUG:
      add(pet.amour,   HUG_LOVE);
      add(pet.humeur,  HUG_MOOD);
      add(pet.fatigue, HUG_FATIGUE);
      break;

    default: break;
  }

  uiSpriteDirty = true;
  uiForceBands  = true;

  // save apr闂佺粯姘ㄩ獮?action importante
  saveNow(now, "action");
}

// ================== Start task ==================
static bool startTask(TaskKind k, uint32_t now) {
  if (!pet.vivant) { setMsg("\xe8\xbf\x99\xe5\x8f\xaa\xe9\xbe\x99\xe6\xb3\xa1\xe6\xb3\xa1\xe5\xb7\xb2\xe7\xa6\xbb\xe5\xbc\x80\xe7\xbd\x97\xe5\xbe\xb7\xe5\xb2\x9b...", now, 2200); return false; }
  if (phase != PHASE_ALIVE && phase != PHASE_RESTREADY) return false;
  if (state == ST_SLEEP && k != TASK_SLEEP) return false;

  // IMPORTANT: LAVER / JOUER passent par mini-jeux -> on refuse ici
  if (k == TASK_WASH || k == TASK_PLAY) return false;

  clearAutoBehavior();
  task.active = true;
  task.kind   = k;
  task.ph     = PH_GO;
  task.doUntil = 0;
  task.startedAt = now;
  task.plannedTotal = 0;
  task.doMs = 0;

  if (k == TASK_EAT) {
    task.targetX = clampf(eatSpotX(), worldMin, worldMax);
    task.returnX = homeX;

    task.doMs = scaleByFatigueAndAge(BASE_EAT_MS);
    task.plannedTotal = task.doMs;

    activityShowFull(now, "\xe5\x89\x8d\xe5\xbe\x80\xe8\xa1\xa5\xe7\xbb\x99\xe7\x82\xb9...");  // 闂備礁鎲￠幐鍝ョ矓鐎靛憡顐芥慨妯块哺閸嬫﹢鏌曟竟顖氭嚀閸╂姊?..
    enterState(ST_WALK, now);
  }
  else if (k == TASK_DRINK) {
    task.targetX = clampf(drinkSpotX(), worldMin, worldMax);
    task.returnX = homeX;

    task.doMs = scaleByFatigueAndAge(BASE_DRINK_MS);
    task.plannedTotal = task.doMs;

    activityShowFull(now, "\xe5\x89\x8d\xe5\xbe\x80\xe9\xa5\xae\xe6\xb0\xb4\xe7\xab\x99...");  // 闂備礁鎲￠幐鍝ョ矓鐎靛憡顐芥慨姗嗗弾濞堢増銇勯弮鍥撻悗鍨矌缁?..
    enterState(ST_WALK, now);
  }
  else if (k == TASK_SLEEP) {
    task.targetX = worldX;
    task.returnX = homeX;
    task.plannedTotal = 1;
    activityVisible = true;
    activityStart = now;
    activityEnd   = now;
    strncpy(activityText, "\xe7\x9d\xa1\xe8\xa7\x89\xe4\xb8\xad...", sizeof(activityText)-1);  // 闂備焦妞挎导鈺呭Χ鎼淬垻绉垮┑?..
    activityText[sizeof(activityText)-1] = 0;

    task.ph = PH_DO;
    enterState(ST_SLEEP, now);
  }
  else {
    task.targetX = worldX;
    task.returnX = homeX;
    uint32_t tDo = (k==TASK_POOP)? scaleByFatigueAndAge(BASE_POOP_MS)
                : (k==TASK_HUG )? scaleByFatigueAndAge(BASE_HUG_MS)
                : 1500;
    task.plannedTotal = tDo;
    task.ph = PH_DO;
    task.doUntil = now + tDo;

    if (k == TASK_POOP) {
      activityStartTask(now, "\xe5\x90\x8e\xe5\x8b\xa4\xe6\xb8\x85\xe7\x90\x86\xe4\xb8\xad...", task.plannedTotal);   // 闂備礁鎲￠懝鐐殽閸濄儮鍋撳鍗炴瀻闁靛棙甯″畷銊╊敍濞戞ǚ鍋撻悙娴嬫?..
    } else if (k == TASK_HUG) {
      activityStartTask(now, "\xe5\x8d\x9a\xe5\xa3\xab\xe9\x99\xaa\xe4\xbc\xb4\xe4\xb8\xad...", task.plannedTotal);  // 闂備礁鎲￠〃鍡涙嚌妤ｅ啯鏅悗锝庡枟閳锋梹銇勮箛鎾虫殭鐎规挸妫楅埥?..
    }

    enterState(ST_SIT, now);
  }

  uiSpriteDirty = true;
  uiForceBands  = true;
  return true;
}

// ================== Update task ==================
static uint32_t lastSleepGainAt = 0;

static void updateTask(uint32_t now) {
  if (!task.active) return;

  if (task.kind == TASK_SLEEP) {
    if (lastSleepGainAt == 0) lastSleepGainAt = now;
    uint32_t dt = now - lastSleepGainAt;
    if (dt >= 100) {
      float sec = (float)dt / 1000.0f;
      pet.fatigue = clamp01f(pet.fatigue + SLEEP_GAIN_PER_SEC * sec);
      lastSleepGainAt = now;
      uiSpriteDirty = true; uiForceBands = true;
    }
    activitySetText("\xe7\x9d\xa1\xe8\xa7\x89\xe4\xb8\xad...");  // 闂備焦妞挎导鈺呭Χ鎼淬垻绉垮┑?..
    return;
  }

  // GO / RETURN (d闂佽偐鍘у畵鍗╝cement)
  if (task.ph == PH_GO || task.ph == PH_RETURN) {
    float goal = (task.ph == PH_GO) ? task.targetX : task.returnX;

    if (fabsf(goal - worldX) < 1.2f) {
      worldX = goal;

      if (task.ph == PH_GO) {
        task.ph = PH_DO;

        if (task.kind == TASK_EAT) {
          if (!berriesLeftAvailable) {
            activityShowFull(now, "\xe8\xa1\xa5\xe7\xbb\x99\xe7\x82\xb9\xe7\xa9\xba\xe4\xba\x86...");  // 闂佽崵鍋炵粙蹇涘磿瀹曞洤鍨濆ù鐓庣摠閸婄兘鏌涢敂璇插箰闁轰礁瀚湁?..
            task.ph = PH_RETURN;
            enterState(ST_WALK, now);
            return;
          }
          activityShowProgress(now, "\xe9\xa2\x86\xe5\x8f\x96\xe9\xa4\x90\xe5\x8c\x85...", task.doMs);  // 濠碘槅鍋呭妯何涘Δ鍕╀汗闁搞儺鐓堝Σ顕€鏌熼柇锕€澧柣?..
          enterState(ST_EAT, now);
          task.doUntil = now + task.doMs;
          return;
        }
        else if (task.kind == TASK_DRINK) {
          if (!puddleVisible) {
            activityShowFull(now, "\xe9\xa5\xae\xe6\xb0\xb4\xe7\xab\x99\xe7\xa9\xba\xe4\xba\x86...");  // 濠德板€栭〃蹇涘窗濡ゅ懎绠板璺虹灱閸楁碍绻涢崱妯虹亶闁轰礁瀚湁?..
            task.ph = PH_RETURN;
            enterState(ST_WALK, now);
            return;
          }
          activityShowProgress(now, "\xe8\xa1\xa5\xe6\xb0\xb4\xe4\xb8\xad...", task.doMs);  // 闂佽崵鍋炵粙蹇涘磿閺屻儱绠板璺侯儑閳?..
          enterState(ST_EAT, now);
          task.doUntil = now + task.doMs;
          return;
        }

      } else {
        task.active = false;
        enterState(ST_SIT, now);
        activityStopIfFree(now);
        uiSpriteDirty = true; uiForceBands = true;
        return;
      }
    } else {
      enterState(ST_WALK, now);
      movingRight = (goal >= worldX);
      float sp = moveSpeedPxPerFrame();
      float dir = movingRight ? 1.0f : -1.0f;
      worldX += dir * sp;
      worldX = clampf(worldX, worldMin, worldMax);
    }

    if (task.ph == PH_GO) {
      if (task.kind == TASK_EAT) activitySetText("\xe5\x89\x8d\xe5\xbe\x80\xe8\xa1\xa5\xe7\xbb\x99\xe7\x82\xb9...");       // 闂備礁鎲￠幐鍝ョ矓鐎靛憡顐芥慨妯块哺閸嬫﹢鏌曟竟顖氭嚀閸╂姊?..
      else if (task.kind == TASK_DRINK) activitySetText("\xe5\x89\x8d\xe5\xbe\x80\xe9\xa5\xae\xe6\xb0\xb4\xe7\xab\x99...");           // 闂備礁鎲￠幐鍝ョ矓鐎靛憡顐芥慨姗嗗弾濞堢増銇勯弮鍥撻悗鍨矌缁?..
    } else if (task.ph == PH_RETURN) {
      if (task.kind == TASK_EAT) activitySetText("\xe8\xa1\xa5\xe7\xbb\x99\xe5\xae\x8c\xe6\x88\x90\xef\xbc\x8c\xe8\xbf\x94\xe5\x9b\x9e...");                 // 闂佽崵鍋炵粙蹇涘磿瀹曞洤鍨濋柟娈垮枓閸嬫捇鎮烽悧鍫熸嫳闂佹悶鍔嶅畝鎼佸极瀹ュ懐鏆嗛柛鎰典簽椤斿姊?..
      else if (task.kind == TASK_DRINK) activitySetText("\xe8\xa1\xa5\xe6\xb0\xb4\xe5\xae\x8c\xe6\x88\x90\xef\xbc\x8c\xe8\xbf\x94\xe5\x9b\x9e...");           // 闂佽崵鍋炵粙蹇涘磿閺屻儱绠板璺哄閸嬫捇鎮烽悧鍫熸嫳闂佹悶鍔嶅畝鎼佸极瀹ュ懐鏆嗛柛鎰典簽椤斿姊?..
    }

    return;
  }

  // DO (manger/boire)
  if (task.ph == PH_DO) {
    if (task.doUntil != 0 && (int32_t)(now - task.doUntil) >= 0) {

      if (task.kind == TASK_EAT) {
        berriesLeftAvailable = false;
        berriesRespawnAt = now + (uint32_t)random(6000, 14000);
      }
      if (task.kind == TASK_DRINK) {
        puddleVisible = false;
        puddleRespawnAt = now + 2000;
      }

      if (task.kind == TASK_EAT) {
        playRTTTLOnce(RTTTL_EAT_INTRO, AUDIO_PRIO_MED);
      } else if (task.kind == TASK_DRINK) {
        playRTTTLOnce(RTTTL_DRINK_INTRO, AUDIO_PRIO_MED);
      } else if (task.kind == TASK_HUG) {
        playRTTTLOnce(RTTTL_HUG_INTRO, AUDIO_PRIO_MED);
      }

      applyTaskEffects(task.kind, now);

      task.ph = PH_RETURN;

      if (task.kind == TASK_EAT) activityShowFull(now, "\xe8\xa1\xa5\xe7\xbb\x99\xe5\xae\x8c\xe6\x88\x90\xef\xbc\x8c\xe8\xbf\x94\xe5\x9b\x9e...");       // 闂佽崵鍋炵粙蹇涘磿瀹曞洤鍨濋柟娈垮枓閸嬫捇鎮烽悧鍫熸嫳闂佹悶鍔嶅畝鎼佸极瀹ュ懐鏆嗛柛鎰典簽椤斿姊?..
      else if (task.kind == TASK_DRINK) activityShowFull(now, "\xe8\xa1\xa5\xe6\xb0\xb4\xe5\xae\x8c\xe6\x88\x90\xef\xbc\x8c\xe8\xbf\x94\xe5\x9b\x9e..."); // 闂佽崵鍋炵粙蹇涘磿閺屻儱绠板璺哄閸嬫捇鎮烽悧鍫熸嫳闂佹悶鍔嶅畝鎼佸极瀹ュ懐鏆嗛柛鎰典簽椤斿姊?..

      enterState(ST_WALK, now);
    }
    return;
  }
}

// ================== Evolution (NON cons闂佽偐鍘у畷鎶瞭if) ==================
static inline bool goodEvolve80() {
  return (pet.faim   >= EVOLVE_THR &&
          pet.soif   >= EVOLVE_THR &&
          pet.hygiene>= EVOLVE_THR &&
          pet.humeur >= EVOLVE_THR &&
          pet.fatigue>= EVOLVE_THR &&
          pet.amour  >= EVOLVE_THR);
}
static void evolutionTick(uint32_t now) {
  if (!pet.vivant) return;
  if (phase != PHASE_ALIVE) return;

  if (goodEvolve80()) pet.evolveProgressMin++;

  uint32_t need = (pet.stage == AGE_JUNIOR) ? EVOLVE_JUNIOR_TO_ADULT_MIN
               : (pet.stage == AGE_ADULTE) ? EVOLVE_ADULT_TO_SENIOR_MIN
               : EVOLVE_SENIOR_TO_REST_MIN;

  if (need == 0) return;

  if (pet.evolveProgressMin >= need) {
    pet.evolveProgressMin = 0;

    if (pet.stage == AGE_JUNIOR) {
      pet.stage = AGE_ADULTE;
      setMsg("\xe6\x88\x90\xe9\x95\xbf\xe8\xae\xb0\xe5\xbd\x95\xe6\x9b\xb4\xe6\x96\xb0: \xe6\x88\x90\xe4\xbd\x93!", now, 2200);  // 闂備胶鎳撻悺銊╁礉濞嗘挸姹查弶鍫氭櫆婵ジ鏌℃径搴㈢《缂佸娼￠弻锟犲炊瑜忛幗鐘绘煛? 闂備胶鎳撻悺銊╁礉閹烘梻绀?
      saveNow(now, "evolve");
    } else if (pet.stage == AGE_ADULTE) {
      pet.stage = AGE_SENIOR;
      setMsg("\xe6\x88\x90\xe9\x95\xbf\xe8\xae\xb0\xe5\xbd\x95\xe6\x9b\xb4\xe6\x96\xb0: \xe7\xa8\xb3\xe5\xae\x9a\xe4\xbd\x93!", now, 2200);  // 闂備胶鎳撻悺銊╁礉濞嗘挸姹查弶鍫氭櫆婵ジ鏌℃径搴㈢《缂佸娼￠弻锟犲炊瑜忛幗鐘绘煛? 缂傚倷绀侀鍛村疮閸ф鍋╅柣鎰靛墯婵?
      saveNow(now, "evolve");
    } else {
      phase = PHASE_RESTREADY;
      uiSpriteDirty = true;
      uiForceBands = true;
      saveNow(now, "restready");
    }
  }
}

static void updateHealthTick(uint32_t now) {
  (void)now;
  if (!pet.vivant) return;

  float ds = 0;
  if (pet.faim    < 15) ds -= 2;
  if (pet.soif    < 15) ds -= 2;
  if (pet.hygiene < 15) ds -= 1;
  if (pet.humeur  < 10) ds -= 1;
  if (pet.fatigue < 10) ds -= 1;
  if (pet.amour   < 10) ds -= 0.5f;

  if (ds < 0) pet.sante = clamp01f(pet.sante + ds);

  if (pet.sante <= 0) {
    handleDeath(now);
  }
}
static void handleDeath(uint32_t now) {
  pet.vivant = false;
  phase = PHASE_TOMB;
  task.active = false;
  task.kind = TASK_NONE;
  appMode = MODE_PET;
  clearAutoBehavior();
  enterState(ST_DEAD, now);
  activityShowFull(now, "\xe5\x8d\x9a\xe5\xa3\xab\xe6\x9c\xaa\xe8\x83\xbd\xe5\xae\x8c\xe6\x88\x90\xe7\x85\xa7\xe6\x8a\xa4\xef\xbc\x8c\xe5\xae\x83\xe7\xa6\xbb\xe5\xbc\x80\xe4\xba\x86\xe7\xbd\x97\xe5\xbe\xb7\xe5\xb2\x9b\xe3\x80\x82");  // 闂備礁鎲￠〃鍡涙嚌妤ｅ啯鏅悗锝庡枛鐎氬銇勮箛鎾存悙闁告ɑ鎸抽幃妤呮偡閻楀牊鎷遍梺鎼炲妽瀹€鎼佸箖闄囬ˇ鍐测攽椤斿搫鐏查柡浣哥Ф娴狅箓鎮剧仦鐐杹缂傚倷绀侀崐浠嬵敋瑜忓Σ鎰板箣閻愭娲搁梺绯曞墲閻熝呯矙閸曨厸鍋撻崹顐ｇ凡闁搞垺鐓￠幆灞解攽鐎ｎ€?
  uiSel = 0;
  uiSpriteDirty = true;
  uiForceBands = true;
  saveNow(now, "dead");
}

static void eraseSavesAndRestart() {
  if (saveReady) {
    if (saveFS.exists(SAVE_A)) saveFS.remove(SAVE_A);
    if (saveFS.exists(SAVE_B)) saveFS.remove(SAVE_B);
    if (saveFS.exists(TMP_A)) saveFS.remove(TMP_A);
    if (saveFS.exists(TMP_B)) saveFS.remove(TMP_B);
  }
  delay(50);
  ESP.restart();
}

// ================== Tick pet (1 minute * SIM_SPEED) ==================
static uint32_t lastPetTick = 0;

static void updatePetTick(uint32_t now) {
  if (!pet.vivant) return;
  if (phase != PHASE_ALIVE) return;

  bool sleeping = (state == ST_SLEEP);
  auto add = [](float &v, float dv){ v = clamp01f(v + dv); };

  if (sleeping) {
    add(pet.faim,    -1.0f);
    add(pet.soif,    -1.0f);
    add(pet.hygiene, -0.5f);
    add(pet.humeur,  +0.2f);
    add(pet.amour,   -0.2f);
    add(pet.sante,   + 0.1f);
  } else {
    add(pet.faim,    AWAKE_HUNGER_D);
    add(pet.soif,    AWAKE_THIRST_D);
    add(pet.hygiene, AWAKE_HYGIENE_D);
    add(pet.humeur,  AWAKE_MOOD_D);
    add(pet.fatigue, AWAKE_FATIGUE_D);
    add(pet.amour,   AWAKE_LOVE_D);
  }

  updateHealthTick(now);

  pet.lungmenCoin += LUNGMEN_COIN_PER_MIN;
  pet.ageMin++;
  evolutionTick(now);

  uiSpriteDirty = true;
  uiForceBands  = true;
}

// ================== RESET ==================
static void resetStatsToDefault() {
  currentScene = SCENE_DORM;
  pet.faim=60; pet.soif=60; pet.hygiene=80; pet.humeur=60;
  pet.fatigue=100; pet.amour=60;
  pet.sante=80; pet.ageMin=0; pet.lungmenCoin=0; pet.vivant=true;
  pet.stage=AGE_JUNIOR;
  pet.evolveProgressMin=0;
  strcpy(petName, "???");
}

static void resetToEgg(uint32_t now) {
  (void)now;
  resetStatsToDefault();
  applySceneConfig(SCENE_DORM, false);
  phase = PHASE_EGG;
  task.active = false;
  state = ST_SIT;
  animIdx = 0;
  nextAnimTick = 0;

  poopVisible = false; poopUntil = 0;

  berriesLeftAvailable = true; berriesRespawnAt = 0;
  puddleVisible = true; puddleRespawnAt = 0;

  activityVisible = false;
  activityText[0] = 0;
  showMsg = false;

  // mini-jeu reset
  appMode = MODE_PET;
  mg.active = false;
  mg.kind = TASK_NONE;
  clearAutoBehavior();
  clearDailyEventQueue();
  lastDailyEventDay = 0;

  uiSel = 0;
  uiSpriteDirty = true; uiForceBands = true;
  audioNextAlertAt = 0;
}
