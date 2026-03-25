#pragma once

// Scene switching and world-position reconciliation.

static void applySceneConfig(SceneId sceneId, bool keepRelativePosition) {
  const SceneConfig& cfg = kSceneConfigs[(int)clampi((int)sceneId, 0, (int)SCENE_COUNT - 1)];

  float relativeX = 0.5f;
  if (worldW > 1.0f) relativeX = clampf(worldX / worldW, 0.0f, 1.0f);

  currentScene = cfg.id;
  worldW = max((float)SW, cfg.worldScreens * (float)SW);
  worldMin = 0.0f;
  worldMax = worldW;
  homeX = clampf(cfg.homeRatio * worldW, 24.0f, worldW - 24.0f);
  bushLeftX = clampf(cfg.supplyLeftX, 8.0f, worldW - 80.0f);
  puddleX = clampf(worldW - cfg.waterRightMargin - (float)sceneWaterPropWidth(), bushLeftX + 72.0f, worldW - 48.0f);

  if (keepRelativePosition) {
    worldX = clampf(relativeX * worldW, worldMin, worldMax);
  } else {
    worldX = homeX;
  }
  camX = clampf(worldX - (float)(SW / 2), 0.0f, worldW - (float)SW);
  uiSpriteDirty = true;
  uiForceBands = true;
}

static inline bool canOpenSceneSelect() {
  return appMode == MODE_PET && phase == PHASE_ALIVE && !task.active && state != ST_SLEEP;
}

static void switchSceneByOffset(int delta, uint32_t now) {
  if (!canOpenSceneSelect()) {
    setMsg("\xe5\xbd\x93\xe5\x89\x8d\xe7\x8a\xb6\xe6\x80\x81\xe6\x97\xa0\xe6\xb3\x95\xe5\x88\x87\xe6\x8d\xa2\xe5\x9c\xba\xe6\x99\xaf", now, 1600);
    return;
  }

  int next = ((int)currentScene + delta) % (int)SCENE_COUNT;
  if (next < 0) next += (int)SCENE_COUNT;

  clearAutoBehavior();
  applySceneConfig((SceneId)next, false);
  enterState(ST_SIT, now);

  char tmp[48];
  snprintf(tmp, sizeof(tmp), "\xe5\xb7\xb2\xe5\x88\x87\xe6\x8d\xa2\xe8\x87\xb3%s", sceneLabel(currentScene));
  setMsg(tmp, now, 1600);
  if (saveReady) saveNow(now, "scene");
}
