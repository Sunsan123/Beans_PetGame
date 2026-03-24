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
- `export` is retained only as historical metadata for migrated assets. Runtime code does not depend on generated headers anymore.
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

Current target:

- `runtime_bundles`: writes runtime pack/index output under `assets/build/runtime`

Runtime bundle layout:

```text
assets/build/runtime/
  manifest.json
  packs/
    <pack>.bpk
  index/
    <pack>.bix
```

- `packs/<pack>.bpk`: RGB565 payload blobs with a small binary header
- `index/<pack>.bix`: per-pack binary metadata index used by the ESP32-side runtime loader
- `manifest.json`: top-level list of runtime packs and file locations

See `assets/docs/runtime-pack-format.md` for the binary pack/index format contract.

Device-side deployment path:

- copy the contents of `assets/build/runtime/` to `/assets/runtime/` on the target SD card or LittleFS volume
- the Arduino runtime now depends on this path directly; there is no legacy header fallback in the main game path
- current runtime integration scope:
  - single-image path: all current single-image art is runtime-driven
  - animation path: all current animated art is runtime-driven, including triceratops state animations, egg hatch frames, and gameplay waste frames
  - cache policy: single-image runtime assets stay resident after first load; animation frames use a small LRU cache on device

One-click SD export on Windows:

```powershell
.\tools\export_runtime_to_sd.cmd
```

- the script is interactive now: it rebuilds runtime bundles, then asks for an SD root or local export folder
- if exactly one removable drive is mounted, pressing Enter exports directly to `<drive>\assets\runtime\`
- if you want a staging directory instead of a live SD card, enter a local path such as `C:\temp\sd_export`

## Validation And Reporting

Run these from the repository root:

```powershell
python Sprites\build_sprites.py --check
python Sprites\build_sprites.py
```

- `--check`: validates catalog/manifests and writes `assets/build/report.json` without generating headers
- default build: validates first, then regenerates runtime bundles and updates `assets/build/report.json`

The report captures:

- build summary and counts
- validation warnings/errors
- generated files
- referenced and orphaned source files
