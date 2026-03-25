# JurassicLifeCN Modules

This folder holds the staged refactor split extracted from `JurassicLifeCN.ino`.

Current modules:

- `JurassicLifePersistenceRtc.h`
  RTC detection, RTC write helpers, storage initialization, save/load, and offline settlement state.

- `JurassicLifeModalFlow.h`
  Offline report, RTC prompt/set flow, scene-select modal flow, and modal input handling.

- `JurassicLifeUiActionState.h`
  Bottom action metadata, labels, and interaction-menu state.

- `JurassicLifeUiRender.h`
  HUD, overlays, bottom band, and world-adjacent UI rendering.

- `JurassicLifeWorldRender.h`
  Scene backdrops, world-band composition, and full-frame rendering.

- `JurassicLifePetSimulation.h`
  Pet task flow, health/evolution ticks, death handling, and reset helpers.

- `JurassicLifePetEvents.h`
  Offline settlement, daily events, and idle/auto-behavior flow.

- `JurassicLifeUiInput.h`
  Touch, encoder, and button routing for gameplay UI.

- `JurassicLifeMiniGame.h`
  Wash/play mini-game state updates and rendering.

- `JurassicLifeSceneState.h`
  Scene configuration, scene labels, and shared world/prop placement state.

- `JurassicLifeSceneFlow.h`
  Scene application and scene-switch flow.

- `JurassicLifeIntro.h`
  Home intro/title screen flow.

- `JurassicLifeLifecycle.h`
  Boot sequencing and per-frame app loop orchestration.

Why these are implementation headers:

- Arduino `.ino` builds rely on a single generated translation unit.
- This refactor keeps runtime behavior stable while reducing the size and responsibility of the main `.ino`.
- Each file is intended to be included exactly once from `JurassicLifeCN.ino`.

Remaining high-value refactor targets:

- Asset/runtime loading service boundary
- A dedicated `AppState` container to reduce cross-module globals
- Splitting touch backends and audio behavior into platform-oriented modules
