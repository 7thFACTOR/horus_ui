"""Build a white Google Material icon atlas PNG + JSON name/rect map.

Fetches every icon available in the official google/material-design-icons
repository, rasterizes them in white, packs them into one PNG and writes a JSON
file mapping lowercase_with_underscores names to rectangles in that PNG.

Requires: pip install resvg-py pillow
Run from anywhere: python generate_material_atlas.py
"""

import io
import json
import re
import urllib.request
from concurrent.futures import ThreadPoolExecutor, as_completed

import resvg_py
from PIL import Image

OUT_DIR = r"D:\horus\themes\icons"
ATLAS_NAME = "material_icons.png"
JSON_NAME = "material_icons.json"
ICON_SIZE = 24
PADDING = 2
COLUMNS = 64

# Icons the level editor demo asks for by name; fail loudly if one goes missing.
REQUIRED_NAMES = [
    "menu",
    "build",
    "account_tree",
    "settings",
    "crop_free",
    "folder_open",
    "article",
    "add",
    "save",
    "undo",
    "redo",
    "visibility",
    "lock",
    "view_in_ar",
    "chevron_right",
    "expand_more",
]

API_BASE = "https://api.github.com/repos/google/material-design-icons/contents/"
RAW_BASE = "https://raw.githubusercontent.com/google/material-design-icons/master/"

# Different icons ship different style folders, so try them in order.
STYLE_SUFFIXES = [
    "/materialicons/24px.svg",
    "/materialiconsoutlined/24px.svg",
    "/materialiconsround/24px.svg",
    "/materialiconssharp/24px.svg",
    "/materialiconstwo_tone/24px.svg",
    "/materialiconstwotone/24px.svg",
    "/24px.svg",
]


def api_json(path):
    req = urllib.request.Request(API_BASE + path, headers={"User-Agent": "horus-icon-builder"})
    with urllib.request.urlopen(req, timeout=60) as response:
        return json.load(response)


def find_icon_bases():
    # The recursive tree API truncates on this repo, so list one level at a time.
    bases = {}
    for category in api_json("src"):
        if category.get("type") != "dir":
            continue
        cat = category["name"]
        for entry in api_json("src/" + cat):
            if entry.get("type") == "dir":
                bases[entry["name"]] = "src/{}/{}".format(cat, entry["name"])
    return bases


def sanitize_name(name):
    name = name.lower()
    name = re.sub(r"[^a-z0-9_]", "_", name)
    name = re.sub(r"_+", "_", name).strip("_")
    return name


def build_name_map(bases):
    mapping = {}
    used = set()

    for original, base in bases.items():
        clean = sanitize_name(original)
        if not clean:
            continue
        if clean in used:
            suffix = 2
            while "{}_{}".format(clean, suffix) in used:
                suffix += 1
            clean = "{}_{}".format(clean, suffix)
        used.add(clean)
        mapping[clean] = base

    for name in REQUIRED_NAMES:
        if name in bases:
            mapping.setdefault(name, bases[name])

    return mapping


def render_white_icon(svg_text):
    # Remove the invisible 24x24 background path, then force every real path white.
    svg_text = re.sub(r'<path[^>]*fill="none"[^>]*/>', "", svg_text)
    png_bytes = resvg_py.svg_to_bytes(
        svg_string=svg_text,
        style_sheet="path{fill:#ffffff}",
        width=ICON_SIZE,
        height=ICON_SIZE,
    )
    icon = Image.open(io.BytesIO(png_bytes)).convert("RGBA")
    bbox = icon.getbbox()
    if not bbox:
        return None
    return icon.crop(bbox)


def fetch_and_render(name, base):
    for suffix in STYLE_SUFFIXES:
        try:
            with urllib.request.urlopen(RAW_BASE + base + suffix, timeout=30) as response:
                svg = response.read().decode("utf-8")
            return name, render_white_icon(svg)
        except Exception:
            continue
    print("failed", name)
    return name, None


def main():
    bases = find_icon_bases()
    names = build_name_map(bases)
    print("found", len(names), "icon directories")

    icons = []
    completed = 0
    with ThreadPoolExecutor(max_workers=16) as pool:
        futures = [pool.submit(fetch_and_render, name, base) for name, base in names.items()]
        for future in as_completed(futures):
            name, icon = future.result()
            completed += 1
            if icon is not None:
                icons.append((name, icon))
            if completed % 250 == 0:
                print("processed", completed, "/", len(futures))

    icons.sort(key=lambda item: item[0])
    print("rendered", len(icons), "icons")

    missing_required = [name for name in REQUIRED_NAMES if name not in {n for n, _ in icons}]
    if missing_required:
        print("MISSING REQUIRED:", ", ".join(missing_required))

    cell = ICON_SIZE + PADDING * 2
    rows = (len(icons) + COLUMNS - 1) // COLUMNS
    atlas = Image.new("RGBA", (cell * COLUMNS, cell * rows), (0, 0, 0, 0))

    entries = []
    for index, (name, icon) in enumerate(icons):
        col = index % COLUMNS
        row = index // COLUMNS
        x = col * cell + PADDING
        y = row * cell + PADDING
        atlas.paste(icon, (x, y))
        entries.append(
            {
                "name": name,
                "rect": [x, y, icon.width, icon.height],
            }
        )

    atlas_path = OUT_DIR + "/" + ATLAS_NAME
    with open(atlas_path, "wb") as f:
        atlas.save(f, format="PNG")

    manifest = {
        "image": ATLAS_NAME,
        "iconSize": ICON_SIZE,
        "iconCount": len(entries),
        "icons": entries,
    }
    with open(OUT_DIR + "/" + JSON_NAME, "w", encoding="utf-8") as f:
        json.dump(manifest, f, indent=2)

    print("wrote", atlas_path, atlas.size, "with", len(entries), "icons")


if __name__ == "__main__":
    main()
