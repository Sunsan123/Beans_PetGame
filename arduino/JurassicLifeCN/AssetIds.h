#pragma once

#include <Arduino.h>

namespace BeansAssets {

enum class AssetId : uint16_t {
  UiScreenTitlePageAccueil = 0,
  SceneCommonPropMountain,
  SceneCommonPropTreeBroadleaf,
  SceneCommonPropTreePine,
  SceneCommonPropBushBerry,
  SceneCommonPropBushPlain,
  SceneCommonPropPuddle,
  SceneCommonPropCloud,
  SceneCommonPropBalloon,
  PropGameplayGraveMarker,
};

enum class AnimationSetId : uint8_t {
  CharacterTriceratopsJunior = 0,
  CharacterTriceratopsAdult,
  CharacterTriceratopsSenior,
  CharacterTriceratopsEggHatch,
  PropGameplayWasteDefault,
};

static inline const char* runtimeAssetId(AssetId id) {
  switch (id) {
    case AssetId::UiScreenTitlePageAccueil:
      return "ui.screen.title.page_accueil";
    case AssetId::SceneCommonPropMountain:
      return "scene.common.prop.mountain";
    case AssetId::SceneCommonPropTreeBroadleaf:
      return "scene.common.prop.tree_broadleaf";
    case AssetId::SceneCommonPropTreePine:
      return "scene.common.prop.tree_pine";
    case AssetId::SceneCommonPropBushBerry:
      return "scene.common.prop.bush_berry";
    case AssetId::SceneCommonPropBushPlain:
      return "scene.common.prop.bush_plain";
    case AssetId::SceneCommonPropPuddle:
      return "scene.common.prop.puddle";
    case AssetId::SceneCommonPropCloud:
      return "scene.common.prop.cloud";
    case AssetId::SceneCommonPropBalloon:
      return "scene.common.prop.balloon";
    case AssetId::PropGameplayGraveMarker:
      return "prop.gameplay.grave_marker";
    default:
      return nullptr;
  }
}

static inline const char* runtimePackId(AssetId id) {
  switch (id) {
    case AssetId::UiScreenTitlePageAccueil:
      return "ui_core";
    case AssetId::SceneCommonPropMountain:
    case AssetId::SceneCommonPropTreeBroadleaf:
    case AssetId::SceneCommonPropTreePine:
    case AssetId::SceneCommonPropBushBerry:
    case AssetId::SceneCommonPropBushPlain:
    case AssetId::SceneCommonPropPuddle:
    case AssetId::SceneCommonPropCloud:
    case AssetId::SceneCommonPropBalloon:
      return "scene_common";
    case AssetId::PropGameplayGraveMarker:
      return "props_gameplay";
    default:
      return nullptr;
  }
}

static inline const char* runtimePackId(AnimationSetId id) {
  switch (id) {
    case AnimationSetId::CharacterTriceratopsJunior:
    case AnimationSetId::CharacterTriceratopsAdult:
    case AnimationSetId::CharacterTriceratopsSenior:
    case AnimationSetId::CharacterTriceratopsEggHatch:
      return "character_triceratops";
    case AnimationSetId::PropGameplayWasteDefault:
      return "props_gameplay";
    default:
      return nullptr;
  }
}

}  // namespace BeansAssets
