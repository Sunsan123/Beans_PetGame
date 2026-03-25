#pragma once

// Scene configuration and shared world-state helpers.
// Kept as a single-include implementation header to match the Arduino build model.

struct SceneConfig {
  SceneId id = SCENE_DORM;
  const char* name = "";
  float worldScreens = 2.0f;
  float homeRatio = 0.5f;
  float supplyLeftX = 24.0f;
  float waterRightMargin = 28.0f;
  uint16_t skyColor = C_SKY;
  uint16_t groundColor = C_GROUND;
  uint16_t groundLineColor = C_GLINE;
  uint16_t accentA = 0xFFFF;
  uint16_t accentB = 0x7BEF;
  uint16_t accentC = 0x39C7;
  bool showMountains = true;
  bool showTrees = true;
  bool showClouds = false;
  bool indoor = false;
};

static const SceneConfig kSceneConfigs[SCENE_COUNT] = {
  { SCENE_DORM,     "\xe5\xae\xbf\xe8\x88\x8d",     2.0f, 0.50f, 24.0f, 30.0f, 0xD6FA, 0xC4A3, 0x93AE, 0xF75F, 0xAD55, 0x6B6D, false, false, false, true  },
  { SCENE_ACTIVITY, "\xe6\xb4\xbb\xe5\x8a\xa8\xe5\xae\xa4", 2.2f, 0.50f, 30.0f, 34.0f, 0xC77C, 0x9E7A, 0x6D76, 0xFFE0, 0x07FF, 0xFD20, false, false, false, true  },
  { SCENE_DECK,     "\xe7\x94\xb2\xe6\x9d\xbf",     2.4f, 0.50f, 26.0f, 30.0f, C_SKY,   C_GROUND, C_GLINE, 0xFFFF, 0xBDF7, 0x39C7, true,  true,  true,  false },
};

static SceneId currentScene = SCENE_DORM;
static float worldW = 640.0f;
static float worldMin = 0.0f;
static float worldMax = 640.0f;

static float homeX = 320.0f;

static float bushLeftX = 20.0f;
static float puddleX = 0.0f;

static inline const SceneConfig& currentSceneConfig() {
  return kSceneConfigs[(int)clampi((int)currentScene, 0, (int)SCENE_COUNT - 1)];
}

static inline const char* sceneLabel(SceneId id) {
  return kSceneConfigs[(int)clampi((int)id, 0, (int)SCENE_COUNT - 1)].name;
}

static inline int sceneSupplyPropWidth() {
  return (currentScene == SCENE_ACTIVITY) ? 30 : 34;
}

static inline int sceneWaterPropWidth() {
  return (currentScene == SCENE_DECK) ? 32 : 28;
}

static bool berriesLeftAvailable = true;
static uint32_t berriesRespawnAt = 0;

static bool puddleVisible = true;
static uint32_t puddleRespawnAt = 0;
