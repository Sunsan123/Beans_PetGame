#pragma once

#include <Arduino.h>

#include "AssetIds.h"

#include "pageaccueil.h"

#define triceratops triJ
#include "annim_junior.h"
#undef triceratops

#define triceratops triA
#include "annim_adul.h"
#undef triceratops

#define triceratops triS
#include "annim_senior.h"
#undef triceratops

#define dino egg
#include "annim_oeuf.h"
#undef dino

#define dino poop
#include "annim_caca.h"
#undef dino

#include "tombe.h"
#include "montagne.h"
#include "sapin.h"
#include "arbre.h"
#include "buissonbaie.h"
#include "buissonsansbaie.h"
#include "flaque_eau_60x25.h"
#include "nuage.h"
#include "ballon.h"

namespace BeansAssets {

struct SingleAssetRef {
  const uint16_t* data;
  uint16_t w;
  uint16_t h;
  uint16_t key;
};

struct AnimationSetRef {
  uint16_t w;
  uint16_t h;
  uint16_t key;
};

static inline const uint16_t* triceratopsFrame_J(triJ::AnimId anim, uint8_t frameIndex) {
  triJ::AnimDesc ad;
  memcpy_P(&ad, &triJ::ANIMS[anim], sizeof(ad));
  if (ad.count == 0) return nullptr;
  frameIndex %= ad.count;
  const uint16_t* framePtr = nullptr;
  memcpy_P(&framePtr, &ad.frames[frameIndex], sizeof(framePtr));
  return framePtr;
}

static inline const uint16_t* triceratopsFrame_A(triA::AnimId anim, uint8_t frameIndex) {
  triA::AnimDesc ad;
  memcpy_P(&ad, &triA::ANIMS[anim], sizeof(ad));
  if (ad.count == 0) return nullptr;
  frameIndex %= ad.count;
  const uint16_t* framePtr = nullptr;
  memcpy_P(&framePtr, &ad.frames[frameIndex], sizeof(framePtr));
  return framePtr;
}

static inline const uint16_t* triceratopsFrame_S(triS::AnimId anim, uint8_t frameIndex) {
  triS::AnimDesc ad;
  memcpy_P(&ad, &triS::ANIMS[anim], sizeof(ad));
  if (ad.count == 0) return nullptr;
  frameIndex %= ad.count;
  const uint16_t* framePtr = nullptr;
  memcpy_P(&framePtr, &ad.frames[frameIndex], sizeof(framePtr));
  return framePtr;
}

static inline uint8_t triceratopsAnimCount_J(triJ::AnimId anim) {
  triJ::AnimDesc ad;
  memcpy_P(&ad, &triJ::ANIMS[anim], sizeof(ad));
  return ad.count;
}

static inline uint8_t triceratopsAnimCount_A(triA::AnimId anim) {
  triA::AnimDesc ad;
  memcpy_P(&ad, &triA::ANIMS[anim], sizeof(ad));
  return ad.count;
}

static inline uint8_t triceratopsAnimCount_S(triS::AnimId anim) {
  triS::AnimDesc ad;
  memcpy_P(&ad, &triS::ANIMS[anim], sizeof(ad));
  return ad.count;
}

static inline SingleAssetRef singleAsset(AssetId id) {
  switch (id) {
    case AssetId::UiScreenTitlePageAccueil:
      return {pageaccueil, pageaccueil_W, pageaccueil_H, pageaccueil_KEY};
    case AssetId::SceneCommonPropMountain:
      return {montagne, montagne_W, montagne_H, montagne_KEY};
    case AssetId::SceneCommonPropTreeBroadleaf:
      return {arbre, arbre_W, arbre_H, arbre_KEY};
    case AssetId::SceneCommonPropTreePine:
      return {sapin, sapin_W, sapin_H, sapin_KEY};
    case AssetId::SceneCommonPropBushBerry:
      return {buissonbaie, buissonbaie_W, buissonbaie_H, buissonbaie_KEY};
    case AssetId::SceneCommonPropBushPlain:
      return {buissonsansbaie, buissonsansbaie_W, buissonsansbaie_H, buissonsansbaie_KEY};
    case AssetId::SceneCommonPropPuddle:
      return {flaque_eau_60x25, flaque_eau_60x25_W, flaque_eau_60x25_H, flaque_eau_60x25_KEY};
    case AssetId::SceneCommonPropCloud:
      return {nuage, nuage_W, nuage_H, nuage_KEY};
    case AssetId::SceneCommonPropBalloon:
      return {ballon, ballon_W, ballon_H, ballon_KEY};
    case AssetId::PropGameplayGraveMarker:
      return {tombe, tombe_W, tombe_H, tombe_KEY};
    default:
      return {nullptr, 0, 0, 0};
  }
}

static inline AnimationSetRef animationSet(AnimationSetId id) {
  switch (id) {
    case AnimationSetId::CharacterTriceratopsJunior:
      return {triJ::W, triJ::H, triJ::KEY};
    case AnimationSetId::CharacterTriceratopsAdult:
      return {triA::W, triA::H, triA::KEY};
    case AnimationSetId::CharacterTriceratopsSenior:
      return {triS::W, triS::H, triS::KEY};
    case AnimationSetId::CharacterTriceratopsEggHatch:
      return {egg::W, egg::H, egg::KEY};
    case AnimationSetId::PropGameplayWasteDefault:
      return {poop::W, poop::H, poop::KEY};
    default:
      return {0, 0, 0};
  }
}

static inline uint8_t triceratopsAnimIdForArt(AgeStage stg, AutoBehaviorArt art) {
  AutoBehaviorArt resolved = (art == AUTO_ART_AUTO) ? AUTO_ART_ASSIE : art;
  if (stg == AGE_JUNIOR) {
    switch (resolved) {
      case AUTO_ART_MARCHE: return (uint8_t)triJ::ANIM_JUNIOR_MARCHE;
      case AUTO_ART_CLIGNE: return (uint8_t)triJ::ANIM_JUNIOR_CLIGNE;
      case AUTO_ART_DODO:   return (uint8_t)triJ::ANIM_JUNIOR_DODO;
      case AUTO_ART_AMOUR:  return (uint8_t)triJ::ANIM_JUNIOR_AMOUR;
      case AUTO_ART_MANGE:  return (uint8_t)triJ::ANIM_JUNIOR_MANGE;
      default:              return (uint8_t)triJ::ANIM_JUNIOR_ASSIE;
    }
  } else if (stg == AGE_ADULTE) {
    switch (resolved) {
      case AUTO_ART_MARCHE: return (uint8_t)triA::ANIM_ADULTE_MARCHE;
      case AUTO_ART_CLIGNE: return (uint8_t)triA::ANIM_ADULTE_CLIGNE;
      case AUTO_ART_DODO:   return (uint8_t)triA::ANIM_ADULTE_DODO;
      case AUTO_ART_AMOUR:  return (uint8_t)triA::ANIM_ADULTE_AMOUR;
      case AUTO_ART_MANGE:  return (uint8_t)triA::ANIM_ADULTE_MANGE;
      default:              return (uint8_t)triA::ANIM_ADULTE_ASSIE;
    }
  }

  switch (resolved) {
    case AUTO_ART_MARCHE: return (uint8_t)triS::ANIM_SENIOR_MARCHE;
    case AUTO_ART_CLIGNE: return (uint8_t)triS::ANIM_SENIOR_CLIGNE;
    case AUTO_ART_DODO:   return (uint8_t)triS::ANIM_SENIOR_DODO;
    case AUTO_ART_AMOUR:  return (uint8_t)triS::ANIM_SENIOR_AMOUR;
    case AUTO_ART_MANGE:  return (uint8_t)triS::ANIM_SENIOR_MANGE;
    default:              return (uint8_t)triS::ANIM_SENIOR_ASSIE;
  }
}

static inline uint8_t triceratopsAnimIdForState(AgeStage stg, TriState st) {
  if (stg == AGE_JUNIOR) {
    switch (st) {
      case ST_SIT:   return (uint8_t)triJ::ANIM_JUNIOR_ASSIE;
      case ST_BLINK: return (uint8_t)triJ::ANIM_JUNIOR_CLIGNE;
      case ST_EAT:   return (uint8_t)triJ::ANIM_JUNIOR_MANGE;
      case ST_SLEEP: return (uint8_t)triJ::ANIM_JUNIOR_DODO;
      default:       return (uint8_t)triJ::ANIM_JUNIOR_MARCHE;
    }
  } else if (stg == AGE_ADULTE) {
    switch (st) {
      case ST_SIT:   return (uint8_t)triA::ANIM_ADULTE_ASSIE;
      case ST_BLINK: return (uint8_t)triA::ANIM_ADULTE_CLIGNE;
      case ST_EAT:   return (uint8_t)triA::ANIM_ADULTE_MANGE;
      case ST_SLEEP: return (uint8_t)triA::ANIM_ADULTE_DODO;
      default:       return (uint8_t)triA::ANIM_ADULTE_MARCHE;
    }
  }

  switch (st) {
    case ST_SIT:   return (uint8_t)triS::ANIM_SENIOR_ASSIE;
    case ST_BLINK: return (uint8_t)triS::ANIM_SENIOR_CLIGNE;
    case ST_EAT:   return (uint8_t)triS::ANIM_SENIOR_MANGE;
    case ST_SLEEP: return (uint8_t)triS::ANIM_SENIOR_DODO;
    default:       return (uint8_t)triS::ANIM_SENIOR_MARCHE;
  }
}

static inline uint8_t triceratopsLoveAnimId(AgeStage stg) {
  if (stg == AGE_JUNIOR) return (uint8_t)triJ::ANIM_JUNIOR_AMOUR;
  if (stg == AGE_ADULTE) return (uint8_t)triA::ANIM_ADULTE_AMOUR;
  return (uint8_t)triS::ANIM_SENIOR_AMOUR;
}

static inline uint8_t triceratopsAnimCount(AgeStage stg, uint8_t animId) {
  if (stg == AGE_JUNIOR) return triceratopsAnimCount_J((triJ::AnimId)animId);
  if (stg == AGE_ADULTE) return triceratopsAnimCount_A((triA::AnimId)animId);
  return triceratopsAnimCount_S((triS::AnimId)animId);
}

static inline const uint16_t* triceratopsFrame(AgeStage stg, uint8_t animId, uint8_t frameIndex) {
  if (stg == AGE_JUNIOR) return triceratopsFrame_J((triJ::AnimId)animId, frameIndex);
  if (stg == AGE_ADULTE) return triceratopsFrame_A((triA::AnimId)animId, frameIndex);
  return triceratopsFrame_S((triS::AnimId)animId, frameIndex);
}

static inline int triceratopsWidth(AgeStage stg) {
  if (stg == AGE_JUNIOR) return (int)triJ::W;
  if (stg == AGE_ADULTE) return (int)triA::W;
  return (int)triS::W;
}

static inline int triceratopsHeight(AgeStage stg) {
  if (stg == AGE_JUNIOR) return (int)triJ::H;
  if (stg == AGE_ADULTE) return (int)triA::H;
  return (int)triS::H;
}

static inline const uint16_t* eggHatchFrame(uint8_t hatchIdx) {
  switch (hatchIdx) {
    case 0: return egg::dino_oeuf_001;
    case 1: return egg::dino_oeuf_002;
    case 2: return egg::dino_oeuf_003;
    default: return egg::dino_oeuf_004;
  }
}

static inline const uint16_t* wasteDefaultFrame(uint8_t frameIndex) {
  switch (frameIndex) {
    case 0: return poop::dino_caca_001;
    case 1: return poop::dino_caca_002;
    default: return poop::dino_caca_003;
  }
}

}  // namespace BeansAssets

#define MONT_W    (::BeansAssets::singleAsset(::BeansAssets::AssetId::SceneCommonPropMountain).w)
#define MONT_H    (::BeansAssets::singleAsset(::BeansAssets::AssetId::SceneCommonPropMountain).h)
#define MONT_IMG  (::BeansAssets::singleAsset(::BeansAssets::AssetId::SceneCommonPropMountain).data)

#define SAPIN_W   (::BeansAssets::singleAsset(::BeansAssets::AssetId::SceneCommonPropTreePine).w)
#define SAPIN_H   (::BeansAssets::singleAsset(::BeansAssets::AssetId::SceneCommonPropTreePine).h)
#define SAPIN_IMG (::BeansAssets::singleAsset(::BeansAssets::AssetId::SceneCommonPropTreePine).data)

#define ARBRE_W   (::BeansAssets::singleAsset(::BeansAssets::AssetId::SceneCommonPropTreeBroadleaf).w)
#define ARBRE_H   (::BeansAssets::singleAsset(::BeansAssets::AssetId::SceneCommonPropTreeBroadleaf).h)
#define ARBRE_IMG (::BeansAssets::singleAsset(::BeansAssets::AssetId::SceneCommonPropTreeBroadleaf).data)

#define BBAIE_W   (::BeansAssets::singleAsset(::BeansAssets::AssetId::SceneCommonPropBushBerry).w)
#define BBAIE_H   (::BeansAssets::singleAsset(::BeansAssets::AssetId::SceneCommonPropBushBerry).h)
#define BBAIE_IMG (::BeansAssets::singleAsset(::BeansAssets::AssetId::SceneCommonPropBushBerry).data)

#define BSANS_W   (::BeansAssets::singleAsset(::BeansAssets::AssetId::SceneCommonPropBushPlain).w)
#define BSANS_H   (::BeansAssets::singleAsset(::BeansAssets::AssetId::SceneCommonPropBushPlain).h)
#define BSANS_IMG (::BeansAssets::singleAsset(::BeansAssets::AssetId::SceneCommonPropBushPlain).data)

#define FLAQUE_W   (::BeansAssets::singleAsset(::BeansAssets::AssetId::SceneCommonPropPuddle).w)
#define FLAQUE_H   (::BeansAssets::singleAsset(::BeansAssets::AssetId::SceneCommonPropPuddle).h)
#define FLAQUE_IMG (::BeansAssets::singleAsset(::BeansAssets::AssetId::SceneCommonPropPuddle).data)

#define NUAGE_W   (::BeansAssets::singleAsset(::BeansAssets::AssetId::SceneCommonPropCloud).w)
#define NUAGE_H   (::BeansAssets::singleAsset(::BeansAssets::AssetId::SceneCommonPropCloud).h)
#define NUAGE_IMG (::BeansAssets::singleAsset(::BeansAssets::AssetId::SceneCommonPropCloud).data)

#define BALLON_W   (::BeansAssets::singleAsset(::BeansAssets::AssetId::SceneCommonPropBalloon).w)
#define BALLON_H   (::BeansAssets::singleAsset(::BeansAssets::AssetId::SceneCommonPropBalloon).h)
#define BALLON_IMG (::BeansAssets::singleAsset(::BeansAssets::AssetId::SceneCommonPropBalloon).data)

#define TOMBE_W   (::BeansAssets::singleAsset(::BeansAssets::AssetId::PropGameplayGraveMarker).w)
#define TOMBE_H   (::BeansAssets::singleAsset(::BeansAssets::AssetId::PropGameplayGraveMarker).h)
#define TOMBE_IMG (::BeansAssets::singleAsset(::BeansAssets::AssetId::PropGameplayGraveMarker).data)

#define EGG_W     (::BeansAssets::animationSet(::BeansAssets::AnimationSetId::CharacterTriceratopsEggHatch).w)
#define EGG_H     (::BeansAssets::animationSet(::BeansAssets::AnimationSetId::CharacterTriceratopsEggHatch).h)

#define WASTE_W   (::BeansAssets::animationSet(::BeansAssets::AnimationSetId::PropGameplayWasteDefault).w)
#define WASTE_H   (::BeansAssets::animationSet(::BeansAssets::AnimationSetId::PropGameplayWasteDefault).h)
