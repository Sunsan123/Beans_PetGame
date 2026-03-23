# Asset System

`assets/` is the authoritative home for game art sources and asset metadata.

## Layout

```text
assets/
  src/           # Human-authored source files (.aseprite, .png, etc.)
  manifests/     # Logical asset definitions grouped by domain
  schemas/       # JSON schema docs for the catalog and manifest files
  build/         # Generated outputs (ignored by git)
```

Recommended domain layout under `src/`:

- `characters/`: playable and NPC characters, stage/state variants, source files
- `scenes/`: backgrounds and reusable scene props
- `props/`: gameplay props that are not scene-specific
- `ui/`: UI screens, buttons, icons, panels
- `furniture/`: placeable or shop-enabled furniture
- `shop/`: shop-only art and promo assets

## Manifest Rules

- Source files are organized for humans.
- Runtime and build tooling should use stable `id` values, not filenames.
- `export` describes legacy outputs such as Arduino headers. It is a compatibility layer, not the logical identity of an asset.
- `pack` groups assets for future runtime bundles. It does not need to match folders.

ID convention:

```text
<domain>.<family>.<variant>...
```

Examples:

- `character.triceratops.junior.walk`
- `scene.common.prop.cloud`
- `ui.screen.title.page_accueil`
- `prop.gameplay.grave_marker`

## Build Flow

`tools/asset_pipeline/build_assets.py` reads `assets/catalog.json`, then loads each domain manifest and builds any requested targets.

Current targets:

- `staging_headers`: writes generated `.h` files under `assets/build/generated_headers`
- `runtime_bundles`: writes runtime pack/index output under `assets/build/runtime`
- `sprites_compat_headers`: writes compatibility output under `Sprites/generated_headers`
- `legacy_headers`: regenerates `.h` sprite headers for `arduino/JurassicLifeCN`

The legacy header target keeps the current game code working while the repository moves toward a runtime asset manager.

Runtime bundle layout:

```text
assets/build/runtime/
  manifest.json
  packs/
    <pack>.bpk
  index/
    <pack>.json
```

- `packs/<pack>.bpk`: RGB565 payload blobs with a small binary header
- `index/<pack>.json`: per-pack asset metadata used by the ESP32-side runtime loader
- `manifest.json`: top-level list of runtime packs and file locations

See `assets/docs/runtime-pack-format.md` for the binary and JSON format contract.

Device-side deployment path:

- copy the contents of `assets/build/runtime/` to `/assets/runtime/` on the target SD card or LittleFS volume
- the first runtime integration currently tries this path for the title screen and falls back to legacy headers if files are missing
- current runtime integration scope:
  - single-image path: title screen and scene static props used by rendering
  - animation path: egg hatch frames and gameplay waste frames
  - cache policy: single-image runtime assets stay resident after first load; animation frames use a small LRU cache on device

## Validation And Reporting

Run these from the repository root:

```powershell
python Sprites\build_sprites.py --check
python Sprites\build_sprites.py
```

- `--check`: validates catalog/manifests and writes `assets/build/report.json` without generating headers
- default build: validates first, then regenerates all configured targets and updates `assets/build/report.json`

The report captures:

- build summary and counts
- validation warnings/errors
- generated files
- referenced and orphaned source files
