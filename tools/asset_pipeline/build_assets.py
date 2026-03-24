#!/usr/bin/env python3

import argparse
import json
import re
import shutil
import struct
from collections import Counter, defaultdict, deque
from copy import deepcopy
from dataclasses import dataclass
from datetime import datetime, timezone
from pathlib import Path

from PIL import Image, ImageSequence

ALLOWED_EXTS = {".png", ".jpg", ".jpeg", ".bmp", ".gif", ".webp"}
KEY_CANDIDATES_565 = [0xF81F, 0x07E0, 0x001F, 0xFFE0, 0x07FF, 0xF800, 0x0010]
BG_TOLERANCE = 22
ALPHA_THRESHOLD = 0
REPORT_PATH = "build/report.json"
PACK_MAGIC = b"BPK1"
PACK_VERSION = 1
PACK_HEADER_SIZE = 32
INDEX_MAGIC = b"BIX1"
INDEX_VERSION = 1
INDEX_HEADER_SIZE = 32
INDEX_ASSET_RECORD_SIZE = 24
INDEX_FRAME_RECORD_SIZE = 12
INDEX_KIND_SINGLE = 1
INDEX_KIND_ANIMATION = 2


def repo_root() -> Path:
    return Path(__file__).resolve().parents[2]


def assets_root() -> Path:
    return repo_root() / "assets"


def rel_repo_path(path: Path) -> str:
    return path.resolve().relative_to(repo_root()).as_posix()


def sanitize_name(name: str) -> str:
    name = name.lower()
    name = re.sub(r"[^a-z0-9_]+", "_", name)
    name = re.sub(r"_+", "_", name).strip("_")
    if not name:
        name = "sprite"
    if name[0].isdigit():
        name = "_" + name
    return name


def rgb888_to_rgb565(r: int, g: int, b: int) -> int:
    return ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3)


def load_first_frame(path: Path) -> Image.Image:
    if path.suffix.lower() not in ALLOWED_EXTS:
        raise ValueError(f"Unsupported image format: {path}")
    im = Image.open(path)
    try:
        frame0 = next(ImageSequence.Iterator(im))
        return frame0.copy()
    except Exception:
        return im.copy()


def color_dist(c1, c2) -> int:
    return abs(c1[0] - c2[0]) + abs(c1[1] - c2[1]) + abs(c1[2] - c2[2])


def guess_background_color(im_rgba: Image.Image):
    w, h = im_rgba.size
    px = im_rgba.load()
    samples = []
    coords = [
        (0, 0),
        (w - 1, 0),
        (0, h - 1),
        (w - 1, h - 1),
        (w // 2, 0),
        (w // 2, h - 1),
        (0, h // 2),
        (w - 1, h // 2),
    ]
    for x, y in coords:
        r, g, b, a = px[x, y]
        samples.append((r, g, b))
    return Counter(samples).most_common(1)[0][0]


def remove_background_to_alpha(im_rgba: Image.Image, tolerance: int) -> Image.Image:
    if im_rgba.mode != "RGBA":
        im_rgba = im_rgba.convert("RGBA")

    w, h = im_rgba.size
    px = im_rgba.load()
    bg = guess_background_color(im_rgba)
    visited = [[False] * w for _ in range(h)]
    q = deque()

    for x in range(w):
        q.append((x, 0))
        q.append((x, h - 1))
    for y in range(h):
        q.append((0, y))
        q.append((w - 1, y))

    def is_bg(x, y) -> bool:
        r, g, b, a = px[x, y]
        if a == 0:
            return True
        return color_dist((r, g, b), bg) <= tolerance

    while q:
        x, y = q.popleft()
        if x < 0 or x >= w or y < 0 or y >= h:
            continue
        if visited[y][x]:
            continue
        visited[y][x] = True
        if not is_bg(x, y):
            continue
        r, g, b, a = px[x, y]
        px[x, y] = (r, g, b, 0)
        q.append((x + 1, y))
        q.append((x - 1, y))
        q.append((x, y + 1))
        q.append((x, y - 1))

    return im_rgba


def trim_transparent(im_rgba: Image.Image):
    if im_rgba.mode != "RGBA":
        im_rgba = im_rgba.convert("RGBA")
    alpha = im_rgba.getchannel("A")
    bbox = alpha.getbbox()
    if bbox is None:
        return im_rgba, None
    return im_rgba.crop(bbox), bbox


def alpha_bbox(im_rgba: Image.Image):
    if im_rgba.mode != "RGBA":
        im_rgba = im_rgba.convert("RGBA")
    return im_rgba.getchannel("A").getbbox()


def union_bbox(bboxes):
    bbox = None
    for current in bboxes:
        if current is None:
            continue
        if bbox is None:
            bbox = current
        else:
            bbox = (
                min(bbox[0], current[0]),
                min(bbox[1], current[1]),
                max(bbox[2], current[2]),
                max(bbox[3], current[3]),
            )
    return bbox


def choose_best_key_for_images(images_rgba):
    opaque_565 = set()
    for im in images_rgba:
        rgba = im.convert("RGBA")
        for r, g, b, a in rgba.getdata():
            if a > ALPHA_THRESHOLD:
                opaque_565.add(rgb888_to_rgb565(r, g, b))
    for key in KEY_CANDIDATES_565:
        if key not in opaque_565:
            return key, False
    return KEY_CANDIDATES_565[0], True


def image_to_rgb565_array(im: Image.Image, key565: int):
    im_rgba = im.convert("RGBA")
    w, h = im_rgba.size
    values = []
    collision = False
    for r, g, b, a in im_rgba.getdata():
        if a <= ALPHA_THRESHOLD:
            values.append(key565)
        else:
            value = rgb888_to_rgb565(r, g, b)
            if value == key565:
                collision = True
            values.append(value)
    return w, h, values, collision


def rgb565_values_to_bytes(values) -> bytes:
    payload = bytearray(len(values) * 2)
    offset = 0
    for value in values:
        payload[offset:offset + 2] = int(value).to_bytes(2, "little")
        offset += 2
    return bytes(payload)


def align4(value: int) -> int:
    return (value + 3) & ~0x03


def write_single_header(out_path: Path, symbol: str, w: int, h: int, key565: int, values):
    def fmt(v):
        return f"0x{v:04X}"

    lines = [
        "#pragma once",
        "#include <Arduino.h>",
        "",
        "// Auto-generated from assets manifest: single image -> RGB565 (KEY transparency)",
        "",
        f"static const uint16_t {symbol}_W = {w};",
        f"static const uint16_t {symbol}_H = {h};",
        f"static const uint16_t {symbol}_KEY = 0x{key565:04X};",
        "",
        f"static const uint16_t {symbol}[{symbol}_W * {symbol}_H] PROGMEM = {{",
    ]

    per_line = 12
    for index in range(0, len(values), per_line):
        chunk = values[index:index + per_line]
        suffix = "," if index + per_line < len(values) else ""
        lines.append("  " + ", ".join(fmt(v) for v in chunk) + suffix)
    lines.extend(["};", ""])
    out_path.write_text("\n".join(lines), encoding="utf-8")


def write_anim_folder_header(out_path: Path, header_name: str, data, key565: int, key_warn: bool):
    def fmt(v):
        return f"0x{v:04X}"

    lines = [
        "#pragma once",
        "#include <Arduino.h>",
        "",
        f"// Auto-generated from assets manifest: animation set '{header_name}' -> RGB565 (KEY transparency)",
        f"// KEY (RGB565): 0x{key565:04X}",
    ]
    if key_warn:
        lines.append("// WARNING: KEY collides with some opaque pixels.")
    lines.append("")

    for char_name in sorted(data.keys(), key=lambda s: s.lower()):
        char_ns = sanitize_name(char_name)
        anims = data[char_name]
        w = anims["_META"]["W"]
        h = anims["_META"]["H"]

        lines.extend(
            [
                f"namespace {char_ns} {{",
                f"  static const uint16_t W = {w};",
                f"  static const uint16_t H = {h};",
                f"  static const uint16_t KEY = 0x{key565:04X};",
                "",
            ]
        )

        for anim_name in [k for k in anims.keys() if k != "_META"]:
            frames = anims[anim_name]
            anim_var = sanitize_name(anim_name)

            for idx, values in frames:
                frame_var = f"{char_ns}_{anim_var}_{idx:03d}"
                lines.append(f"  static const uint16_t {frame_var}[W * H] PROGMEM = {{")
                per_line = 12
                for offset in range(0, len(values), per_line):
                    chunk = values[offset:offset + per_line]
                    suffix = "," if offset + per_line < len(values) else ""
                    lines.append("    " + ", ".join(fmt(v) for v in chunk) + suffix)
                lines.append("  };")
                lines.append("")

            ptrs = [f"{char_ns}_{anim_var}_{idx:03d}" for idx, _ in frames]
            lines.append(f"  static const uint16_t* const {char_ns}_{anim_var}_frames[] PROGMEM = {{")
            lines.append("    " + ", ".join(ptrs))
            lines.append("  };")
            lines.append(f"  static const uint8_t {char_ns}_{anim_var}_count = {len(frames)};")
            lines.append("")

        anim_list = [k for k in anims.keys() if k != "_META"]
        lines.append("  enum AnimId {")
        for idx, anim_name in enumerate(anim_list):
            suffix = "," if idx < len(anim_list) - 1 else ""
            lines.append(f"    ANIM_{sanitize_name(anim_name).upper()}{suffix}")
        lines.append("  };")
        lines.append("")
        lines.append("  struct AnimDesc {")
        lines.append("    const uint16_t* const* frames;")
        lines.append("    uint8_t count;")
        lines.append("  };")
        lines.append("")
        lines.append("  static const AnimDesc ANIMS[] PROGMEM = {")
        for idx, anim_name in enumerate(anim_list):
            anim_var = sanitize_name(anim_name)
            suffix = "," if idx < len(anim_list) - 1 else ""
            lines.append(f"    {{ {char_ns}_{anim_var}_frames, {char_ns}_{anim_var}_count }}{suffix}")
        lines.append("  };")
        lines.append("")
        lines.append(f"}} // namespace {char_ns}")
        lines.append("")

    out_path.write_text("\n".join(lines), encoding="utf-8")


def merge_dict(base, override):
    result = deepcopy(base)
    for key, value in override.items():
        if isinstance(value, dict) and isinstance(result.get(key), dict):
            result[key] = merge_dict(result[key], value)
        else:
            result[key] = value
    return result


def load_json(path: Path):
    return json.loads(path.read_text(encoding="utf-8"))


@dataclass
class ValidationMessage:
    level: str
    message: str
    path: str | None = None
    asset_id: str | None = None

    def to_dict(self):
        payload = {"level": self.level, "message": self.message}
        if self.path:
            payload["path"] = self.path
        if self.asset_id:
            payload["assetId"] = self.asset_id
        return payload


class ValidationState:
    def __init__(self):
        self.messages: list[ValidationMessage] = []

    def error(self, message: str, path: str | None = None, asset_id: str | None = None):
        self.messages.append(ValidationMessage("error", message, path, asset_id))

    def warning(self, message: str, path: str | None = None, asset_id: str | None = None):
        self.messages.append(ValidationMessage("warning", message, path, asset_id))

    def errors(self):
        return [msg for msg in self.messages if msg.level == "error"]

    def warnings(self):
        return [msg for msg in self.messages if msg.level == "warning"]


def normalize_asset_kind(asset, state: ValidationState):
    kind = asset.get("kind")
    if kind not in {"single", "animation"}:
        state.error(f"Unsupported asset kind '{kind}'", asset_id=asset.get("id"))
    return kind


def normalize_source_path(rel_path: str | None) -> str | None:
    if not rel_path:
        return None
    return rel_path.replace("\\", "/")


def validate_processing(asset, state: ValidationState):
    processing = asset.get("processing", {})
    trim = processing.get("trim", False)
    bg_removal = processing.get("bgRemoval", "auto")
    if trim not in {True, False, "none", "union"}:
        state.error(f"Unsupported trim mode '{trim}'", asset_id=asset.get("id"))
    if bg_removal not in {"auto", "never"}:
        state.error(f"Unsupported bgRemoval mode '{bg_removal}'", asset_id=asset.get("id"))


def validate_asset_paths(asset, state: ValidationState, referenced_sources: set[str]):
    root = assets_root()
    kind = normalize_asset_kind(asset, state)
    validate_processing(asset, state)

    def check_file(rel_path: str | None, field_name: str, allowed_exts: set[str] | None = None, warn_only: bool = False):
        rel_path = normalize_source_path(rel_path)
        if not rel_path:
            if warn_only:
                return
            state.error(f"Missing {field_name}", path=asset["_manifest"], asset_id=asset.get("id"))
            return
        full_path = root / rel_path
        referenced_sources.add(rel_path)
        if not full_path.exists():
            state.error(f"{field_name} not found: {rel_path}", path=rel_path, asset_id=asset.get("id"))
            return
        if not full_path.is_file():
            state.error(f"{field_name} is not a file: {rel_path}", path=rel_path, asset_id=asset.get("id"))
            return
        if allowed_exts and full_path.suffix.lower() not in allowed_exts:
            state.error(
                f"{field_name} has unsupported extension '{full_path.suffix}'",
                path=rel_path,
                asset_id=asset.get("id"),
            )

    if kind == "single":
        check_file(asset.get("source"), "source", ALLOWED_EXTS)
    elif kind == "animation":
        frames = asset.get("frames")
        if not isinstance(frames, list) or not frames:
            state.error("Animation asset must define a non-empty frames list", asset_id=asset.get("id"))
        else:
            for index, frame in enumerate(frames, start=1):
                check_file(frame, f"frame {index}", ALLOWED_EXTS)
        if not asset.get("character"):
            state.error("Animation asset must define character", asset_id=asset.get("id"))
        if not asset.get("animation"):
            state.error("Animation asset must define animation", asset_id=asset.get("id"))

    check_file(asset.get("sourceAseprite"), "sourceAseprite", {".aseprite"}, warn_only=True)


def collect_assets(catalog_path: Path, state: ValidationState):
    catalog = load_json(catalog_path)
    assets = []
    manifest_root = catalog_path.parent
    manifest_paths = catalog.get("manifests", [])
    if len(manifest_paths) != len(set(manifest_paths)):
        state.error("Catalog contains duplicate manifest entries", path=rel_repo_path(catalog_path))

    for rel_path in manifest_paths:
        manifest_path = (manifest_root / rel_path).resolve()
        if not manifest_path.exists():
            state.error("Manifest file not found", path=rel_path)
            continue
        manifest = load_json(manifest_path)
        defaults = manifest.get("defaults", {})
        if "assets" not in manifest or not isinstance(manifest["assets"], list):
            state.error("Manifest missing assets array", path=rel_repo_path(manifest_path))
            continue
        for asset in manifest["assets"]:
            merged = merge_dict(defaults, asset)
            merged["_manifest"] = rel_repo_path(manifest_path)
            merged["_group"] = manifest.get("group", "")
            merged["_domain"] = manifest.get("domain", "")
            merged["_pack"] = merged.get("pack", manifest.get("pack"))
            assets.append(merged)

    return catalog, assets


def validate_catalog_and_assets(catalog_path: Path):
    state = ValidationState()
    catalog, assets = collect_assets(catalog_path, state)
    referenced_sources: set[str] = set()
    ids_seen = {}
    single_headers = {}
    single_symbols = {}
    animation_keys = {}
    animation_headers = defaultdict(list)
    single_export_headers = set()

    if not catalog.get("targets"):
        state.error("Catalog must define at least one target", path=rel_repo_path(catalog_path))

    for asset in assets:
        asset_id = asset.get("id")
        if not asset_id:
            state.error("Asset missing id", path=asset["_manifest"])
            continue
        if asset_id in ids_seen:
            state.error(f"Duplicate asset id '{asset_id}'", path=asset["_manifest"], asset_id=asset_id)
        else:
            ids_seen[asset_id] = asset["_manifest"]

        export = asset.get("export", {})
        header = export.get("header")
        if not header:
            state.error("Asset missing export.header", path=asset["_manifest"], asset_id=asset_id)
        if not asset.get("_pack"):
            state.error("Asset missing pack", path=asset["_manifest"], asset_id=asset_id)

        kind = normalize_asset_kind(asset, state)
        validate_asset_paths(asset, state, referenced_sources)

        if kind == "single" and header:
            symbol = export.get("symbol") or sanitize_name(Path(header).stem)
            if header in single_headers:
                state.error(
                    f"Single assets share export header '{header}'",
                    path=asset["_manifest"],
                    asset_id=asset_id,
                )
            else:
                single_headers[header] = asset_id
            if symbol in single_symbols:
                state.error(
                    f"Single assets share export symbol '{symbol}'",
                    path=asset["_manifest"],
                    asset_id=asset_id,
                )
            else:
                single_symbols[symbol] = asset_id
            single_export_headers.add(header)

        if kind == "animation" and header:
            key = (header, asset.get("character"), asset.get("animation"))
            if key in animation_keys:
                state.error(
                    f"Animation header '{header}' already defines character/animation pair {key[1:]!r}",
                    path=asset["_manifest"],
                    asset_id=asset_id,
                )
            else:
                animation_keys[key] = asset_id
            animation_headers[header].append(asset)

    animation_header_names = set(animation_headers.keys())
    overlap_headers = sorted(single_export_headers & animation_header_names)
    for header in overlap_headers:
        state.error(f"Header '{header}' is used by both single and animation assets")

    for header, grouped_assets in animation_headers.items():
        trim_modes = {asset.get("processing", {}).get("trim", False) for asset in grouped_assets}
        bg_modes = {asset.get("processing", {}).get("bgRemoval", "auto") for asset in grouped_assets}
        if len(trim_modes) != 1:
            state.error(f"Animation header '{header}' mixes trim modes")
        if len(bg_modes) != 1:
            state.error(f"Animation header '{header}' mixes bgRemoval modes")

    source_root = assets_root() / catalog.get("sourceRoot", "src")
    all_source_files = set()
    if source_root.exists():
        for path in source_root.rglob("*"):
            if path.is_file() and path.name != ".gitkeep":
                all_source_files.add(path.relative_to(assets_root()).as_posix())

    orphans = sorted(all_source_files - referenced_sources)
    for orphan in orphans:
        state.warning("Source file is not referenced by any manifest", path=orphan)

    legacy_source_files = []
    for base in [repo_root() / "Sprites" / "single", repo_root() / "Sprites" / "anims"]:
        if base.exists():
            legacy_source_files.extend([path for path in base.rglob("*") if path.is_file() and path.suffix.lower() != ".md"])
    if legacy_source_files:
        for path in sorted(legacy_source_files):
            state.warning("Legacy source file remains under Sprites/", path=rel_repo_path(path))

    return catalog, assets, state, sorted(referenced_sources), sorted(all_source_files), orphans


def resolve_processing(asset):
    processing = asset.get("processing", {})
    trim = processing.get("trim", False)
    bg_removal = processing.get("bgRemoval", "auto")
    return trim, bg_removal


def maybe_remove_bg(im: Image.Image, bg_removal: str) -> Image.Image:
    rgba = im.convert("RGBA")
    if bg_removal != "auto":
        return rgba
    alpha = rgba.getchannel("A")
    min_alpha, _ = alpha.getextrema()
    if min_alpha != 0:
        return remove_background_to_alpha(rgba, tolerance=BG_TOLERANCE)
    return rgba


def process_single_runtime_asset(asset):
    root = assets_root()
    source = root / asset["source"]
    trim, bg_removal = resolve_processing(asset)
    image = maybe_remove_bg(load_first_frame(source), bg_removal)
    if trim is True:
        image, _ = trim_transparent(image)

    key565, key_warn = choose_best_key_for_images([image])
    width, height, values, collision = image_to_rgb565_array(image, key565)
    return {
        "width": width,
        "height": height,
        "key565": key565,
        "payload": rgb565_values_to_bytes(values),
        "warn": key_warn or collision,
    }


def process_animation_runtime_asset(asset):
    root = assets_root()
    trim_mode, bg_removal = resolve_processing(asset)
    loaded_frames = []

    for frame_index, rel_path in enumerate(asset.get("frames", []), start=0):
        image = maybe_remove_bg(load_first_frame(root / rel_path), bg_removal)
        loaded_frames.append((frame_index, image))

    if trim_mode == "union":
        bbox = union_bbox(alpha_bbox(image) for _, image in loaded_frames)
        if bbox is not None:
            loaded_frames = [(frame_index, image.crop(bbox)) for frame_index, image in loaded_frames]

    key565, key_warn = choose_best_key_for_images([image for _, image in loaded_frames])
    width = 0
    height = 0
    frames = []
    any_collision = False

    for frame_index, image in loaded_frames:
        frame_width, frame_height, values, collision = image_to_rgb565_array(image, key565)
        if width == 0 and height == 0:
            width = frame_width
            height = frame_height
        elif width != frame_width or height != frame_height:
            raise RuntimeError(
                f"Inconsistent frame size for '{asset['id']}': {(frame_width, frame_height)} vs {(width, height)}"
            )
        frames.append(
            {
                "index": frame_index,
                "width": frame_width,
                "height": frame_height,
                "payload": rgb565_values_to_bytes(values),
            }
        )
        any_collision = any_collision or collision

    return {
        "width": width,
        "height": height,
        "key565": key565,
        "frames": frames,
        "warn": key_warn or any_collision,
    }


def build_single_assets(single_assets, out_dirs):
    outputs = []
    root = assets_root()
    for asset in single_assets:
        source = root / asset["source"]
        trim, bg_removal = resolve_processing(asset)
        im = maybe_remove_bg(load_first_frame(source), bg_removal)
        if trim is True:
            im, _ = trim_transparent(im)

        key565, key_warn = choose_best_key_for_images([im])
        w, h, values, collision = image_to_rgb565_array(im, key565)
        symbol = asset["export"].get("symbol") or sanitize_name(Path(asset["export"]["header"]).stem)
        header_name = asset["export"]["header"]

        for out_dir in out_dirs:
            out_dir.mkdir(parents=True, exist_ok=True)
            out_path = out_dir / header_name
            write_single_header(out_path, symbol, w, h, key565, values)
            outputs.append(rel_repo_path(out_path))

        print(f"[SINGLE OK] {asset['id']} -> {header_name}" + (" [warn]" if key_warn or collision else ""))
    return outputs


def build_animation_assets(animation_assets, out_dirs):
    outputs = []
    root = assets_root()
    grouped_assets = defaultdict(list)
    for asset in animation_assets:
        grouped_assets[asset["export"]["header"]].append(asset)

    for header_name, assets in grouped_assets.items():
        trim_mode = {resolve_processing(asset)[0] for asset in assets}.pop()
        bg_removal = {resolve_processing(asset)[1] for asset in assets}.pop()
        loaded = []
        images_for_key = []

        for asset in assets:
            for idx, rel_path in enumerate(asset.get("frames", []), start=1):
                source = root / rel_path
                im = maybe_remove_bg(load_first_frame(source), bg_removal)
                loaded.append((asset["character"], asset["animation"], idx, im))
                images_for_key.append(im)

        if trim_mode == "union":
            bbox = union_bbox(alpha_bbox(im) for _, _, _, im in loaded)
            if bbox is not None:
                loaded = [(c, a, i, im.crop(bbox)) for c, a, i, im in loaded]

        key565, key_warn = choose_best_key_for_images(images_for_key)
        data = {}
        any_collision = False

        for character, animation_name, idx, im in loaded:
            w, h, values, collision = image_to_rgb565_array(im, key565)
            any_collision = any_collision or collision
            if character not in data:
                data[character] = {"_META": {"W": w, "H": h}}
            elif data[character]["_META"]["W"] != w or data[character]["_META"]["H"] != h:
                raise RuntimeError(
                    f"Inconsistent frame size for '{character}' in {header_name}: "
                    f"{(w, h)} vs {(data[character]['_META']['W'], data[character]['_META']['H'])}"
                )
            data[character].setdefault(animation_name, []).append((idx, values))

        for character in data:
            for animation_name in [name for name in data[character] if name != "_META"]:
                data[character][animation_name] = sorted(data[character][animation_name], key=lambda item: item[0])

        for out_dir in out_dirs:
            out_dir.mkdir(parents=True, exist_ok=True)
            out_path = out_dir / header_name
            write_anim_folder_header(out_path, header_name, data, key565, key_warn or any_collision)
            outputs.append(rel_repo_path(out_path))

        print(f"[ANIM OK] {header_name}" + (" [warn]" if key_warn or any_collision else ""))
    return outputs


def write_pack_header(file_obj, asset_count: int, payload_bytes: int):
    header = struct.pack(
        "<4sHHIIIIII",
        PACK_MAGIC,
        PACK_VERSION,
        PACK_HEADER_SIZE,
        asset_count,
        payload_bytes,
        0,
        0,
        0,
        0,
    )
    file_obj.seek(0)
    file_obj.write(header)


def build_binary_pack_index_bytes(asset_entries):
    frame_entries = []
    for entry in asset_entries:
        if entry["kind"] == INDEX_KIND_ANIMATION:
            frame_entries.extend(entry["frames"])

    string_base_offset = (
        INDEX_HEADER_SIZE
        + len(asset_entries) * INDEX_ASSET_RECORD_SIZE
        + len(frame_entries) * INDEX_FRAME_RECORD_SIZE
    )

    string_table = bytearray()
    string_offsets = {}

    def intern_string(value: str) -> int:
        if value in string_offsets:
            return string_offsets[value]
        offset = string_base_offset + len(string_table)
        encoded = value.encode("utf-8") + b"\x00"
        string_offsets[value] = offset
        string_table.extend(encoded)
        return offset

    asset_blob = bytearray()
    frame_blob = bytearray()
    next_frame_index = 0

    for entry in asset_entries:
        id_offset = intern_string(entry["id"])
        if entry["kind"] == INDEX_KIND_SINGLE:
            asset_blob.extend(
                struct.pack(
                    "<IHHHHHHII",
                    id_offset,
                    INDEX_KIND_SINGLE,
                    0,
                    entry["key565"],
                    entry["width"],
                    entry["height"],
                    0,
                    entry["payloadOffset"],
                    entry["payloadLength"],
                )
            )
        elif entry["kind"] == INDEX_KIND_ANIMATION:
            frames = entry["frames"]
            asset_blob.extend(
                struct.pack(
                    "<IHHHHHHII",
                    id_offset,
                    INDEX_KIND_ANIMATION,
                    0,
                    entry["key565"],
                    entry["width"],
                    entry["height"],
                    len(frames),
                    next_frame_index,
                    0,
                )
            )
            for frame in frames:
                frame_blob.extend(
                    struct.pack(
                        "<HHII",
                        frame["width"],
                        frame["height"],
                        frame["payloadOffset"],
                        frame["payloadLength"],
                    )
                )
            next_frame_index += len(frames)
        else:
            raise RuntimeError(f"Unsupported binary index kind: {entry['kind']}")

    header = struct.pack(
        "<4sHHIIIIII",
        INDEX_MAGIC,
        INDEX_VERSION,
        INDEX_HEADER_SIZE,
        len(asset_entries),
        len(frame_entries),
        INDEX_ASSET_RECORD_SIZE,
        INDEX_FRAME_RECORD_SIZE,
        string_base_offset,
        0,
    )

    return header + bytes(asset_blob) + bytes(frame_blob) + bytes(string_table)


def build_runtime_bundles(assets, out_dir: Path):
    outputs = []
    packs_dir = out_dir / "packs"
    index_dir = out_dir / "index"
    if packs_dir.exists():
        shutil.rmtree(packs_dir)
    if index_dir.exists():
        shutil.rmtree(index_dir)
    packs_dir.mkdir(parents=True, exist_ok=True)
    index_dir.mkdir(parents=True, exist_ok=True)
    manifest_path = out_dir / "manifest.json"
    if manifest_path.exists():
        manifest_path.unlink()

    grouped_assets = defaultdict(list)
    for asset in assets:
        grouped_assets[asset["_pack"]].append(asset)

    manifest_packs = []

    for pack_id in sorted(grouped_assets.keys()):
        pack_assets = sorted(grouped_assets[pack_id], key=lambda asset: asset["id"])
        pack_path = packs_dir / f"{pack_id}.bpk"
        index_path = index_dir / f"{pack_id}.bix"
        asset_entries = []
        warning_seen = False

        with pack_path.open("wb") as pack_file:
            pack_file.write(b"\x00" * PACK_HEADER_SIZE)

            for asset in pack_assets:
                kind = asset["kind"]

                if kind == "single":
                    processed = process_single_runtime_asset(asset)
                    payload_offset = align4(pack_file.tell())
                    padding = payload_offset - pack_file.tell()
                    if padding:
                        pack_file.write(b"\x00" * padding)

                    payload = processed["payload"]
                    pack_file.write(payload)
                    warning_seen = warning_seen or processed["warn"]
                    asset_entries.append(
                        {
                            "id": asset["id"],
                            "kind": INDEX_KIND_SINGLE,
                            "key565": processed["key565"],
                            "width": processed["width"],
                            "height": processed["height"],
                            "payloadOffset": payload_offset,
                            "payloadLength": len(payload),
                        }
                    )
                elif kind == "animation":
                    processed = process_animation_runtime_asset(asset)
                    warning_seen = warning_seen or processed["warn"]
                    frame_entries = []

                    for frame in processed["frames"]:
                        payload_offset = align4(pack_file.tell())
                        padding = payload_offset - pack_file.tell()
                        if padding:
                            pack_file.write(b"\x00" * padding)

                        payload = frame["payload"]
                        pack_file.write(payload)
                        frame_entries.append(
                            {
                                "index": frame["index"],
                                "width": frame["width"],
                                "height": frame["height"],
                                "payloadOffset": payload_offset,
                                "payloadLength": len(payload),
                            }
                        )

                    asset_entries.append(
                        {
                            "id": asset["id"],
                            "kind": INDEX_KIND_ANIMATION,
                            "key565": processed["key565"],
                            "width": processed["width"],
                            "height": processed["height"],
                            "frames": frame_entries,
                        }
                    )
                else:
                    raise RuntimeError(f"Unsupported runtime asset kind '{kind}'")

            payload_bytes = pack_file.tell() - PACK_HEADER_SIZE
            write_pack_header(pack_file, len(asset_entries), payload_bytes)

        index_path.write_bytes(build_binary_pack_index_bytes(asset_entries))

        outputs.append(rel_repo_path(pack_path))
        outputs.append(rel_repo_path(index_path))
        manifest_packs.append(
            {
                "packId": pack_id,
                "packFile": f"packs/{pack_id}.bpk",
                "indexFile": f"index/{pack_id}.bix",
                "assetCount": len(asset_entries),
            }
        )

        print(f"[PACK OK] {pack_id}" + (" [warn]" if warning_seen else ""))

    manifest_payload = {
        "schemaVersion": 1,
        "packFormatVersion": PACK_VERSION,
        "packs": manifest_packs,
    }
    manifest_path.write_text(json.dumps(manifest_payload, indent=2, ensure_ascii=False) + "\n", encoding="utf-8")
    outputs.append(rel_repo_path(manifest_path))

    return outputs


def resolve_targets(catalog, target_names):
    targets = catalog["targets"]
    if target_names == ["all"]:
        selected = list(targets.keys())
    else:
        missing = [name for name in target_names if name not in targets]
        if missing:
            raise ValueError(f"Unknown target(s): {', '.join(missing)}")
        selected = target_names

    resolved = []
    for target_name in selected:
        target = targets[target_name]
        target_type = target["type"]
        if target_type not in {"arduino_headers", "runtime_bundle"}:
            raise ValueError(f"Unsupported target type: {target_type}")
        resolved.append(
            {
                "name": target_name,
                "type": target_type,
                "outDir": (assets_root() / target["outDir"]).resolve(),
            }
        )
    return resolved


def write_report(catalog_path: Path, catalog, assets, state: ValidationState, referenced_sources, all_sources, orphans, built_targets, generated_files):
    build_dir = assets_root() / catalog.get("buildRoot", "build")
    build_dir.mkdir(parents=True, exist_ok=True)
    report_path = build_dir / "report.json"

    payload = {
        "generatedAtUtc": datetime.now(timezone.utc).isoformat(),
        "catalog": rel_repo_path(catalog_path),
        "summary": {
            "assetCount": len(assets),
            "singleCount": sum(1 for asset in assets if asset.get("kind") == "single"),
            "animationCount": sum(1 for asset in assets if asset.get("kind") == "animation"),
            "errorCount": len(state.errors()),
            "warningCount": len(state.warnings()),
            "referencedSourceCount": len(referenced_sources),
            "sourceFileCount": len(all_sources),
            "orphanSourceCount": len(orphans),
        },
        "targets": [
            {
                "name": target["name"],
                "type": target["type"],
                "outDir": rel_repo_path(target["outDir"]),
            }
            for target in built_targets
        ],
        "assets": [
            {
                "id": asset.get("id"),
                "kind": asset.get("kind"),
                "manifest": asset.get("_manifest"),
                "pack": asset.get("_pack"),
                "exportHeader": asset.get("export", {}).get("header"),
            }
            for asset in assets
        ],
        "generatedFiles": generated_files,
        "referencedSources": referenced_sources,
        "orphanSources": orphans,
        "messages": [msg.to_dict() for msg in state.messages],
    }

    report_path.write_text(json.dumps(payload, indent=2, ensure_ascii=False) + "\n", encoding="utf-8")
    return report_path


def main():
    parser = argparse.ArgumentParser(description="Build BeansPetGame assets from manifests.")
    parser.add_argument(
        "--catalog",
        default=str(assets_root() / "catalog.json"),
        help="Path to assets/catalog.json",
    )
    parser.add_argument(
        "--target",
        action="append",
        default=["all"],
        help="Build target name from assets/catalog.json. Repeat for multiple targets.",
    )
    parser.add_argument(
        "--check",
        action="store_true",
        help="Validate catalog/manifests and write assets/build/report.json without generating headers.",
    )
    args = parser.parse_args()

    if args.target != ["all"] and "all" in args.target:
        args.target = [target for target in args.target if target != "all"]

    catalog_path = Path(args.catalog).resolve()
    catalog, assets, state, referenced_sources, all_sources, orphans = validate_catalog_and_assets(catalog_path)

    built_targets = []
    generated_files = []
    if not state.errors() and not args.check:
        built_targets = resolve_targets(catalog, args.target)
        header_dirs = [target["outDir"] for target in built_targets if target["type"] == "arduino_headers"]
        runtime_dirs = [target["outDir"] for target in built_targets if target["type"] == "runtime_bundle"]
        singles = [asset for asset in assets if asset.get("kind") == "single"]
        animations = [asset for asset in assets if asset.get("kind") == "animation"]
        if header_dirs:
            generated_files.extend(build_single_assets(singles, header_dirs))
            generated_files.extend(build_animation_assets(animations, header_dirs))
        for runtime_dir in runtime_dirs:
            generated_files.extend(build_runtime_bundles(assets, runtime_dir))

    report_path = write_report(
        catalog_path,
        catalog,
        assets,
        state,
        referenced_sources,
        all_sources,
        orphans,
        built_targets,
        generated_files,
    )

    for warning in state.warnings():
        prefix = f"{warning.path}: " if warning.path else ""
        print(f"[WARN] {prefix}{warning.message}")

    if state.errors():
        for error in state.errors():
            prefix = f"{error.path}: " if error.path else ""
            print(f"[ERROR] {prefix}{error.message}")
        raise SystemExit(1)

    if args.check:
        print(f"\nValidation OK. Report: {rel_repo_path(report_path)}")
    else:
        print(f"\nDone. Report: {rel_repo_path(report_path)}")


if __name__ == "__main__":
    main()
