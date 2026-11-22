"""Build a white Google Material icon atlas PNG + JSON name/rect map.

Reads the SVGs cached in the local svg/ directory, rasterizes them in white,
packs them into one PNG and writes a JSON file mapping lowercase_with_underscores
names to rectangles in that PNG. Regeneration needs no network access.

Usage:
  python generate_material_atlas.py [icon_size]
  python generate_material_atlas.py --size 32

Icon size defaults to 24 pixels.

Requires: pip install resvg-py pillow
Run from anywhere: python generate_material_atlas.py
"""

import argparse
import io
import json
import re
from concurrent.futures import ThreadPoolExecutor, as_completed
from pathlib import Path

import resvg_py
from PIL import Image

OUT_DIR = Path(__file__).resolve().parent
SVG_DIR = OUT_DIR / "svg"
ATLAS_NAME = "material_icons.png"
JSON_NAME = "material_icons.json"
DEFAULT_ICON_SIZE = 24
PADDING = 2
COLUMNS = 64

SHAPE_TAGS = "path|rect|polygon|circle|ellipse|line|polyline"

# Icons the examples ask for by name; fail loudly if one goes missing.
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
    "attach_file",
    "fact_check",
    "layers",
]


def parse_args():
    parser = argparse.ArgumentParser(
        description="Build the white Material icon atlas from local SVGs.")
    parser.add_argument(
        "icon_size",
        nargs="?",
        type=int,
        default=DEFAULT_ICON_SIZE,
        help="icon size in pixels (default: %(default)s)",
    )
    parser.add_argument(
        "-s",
        "--size",
        dest="size_override",
        type=int,
        default=None,
        help="icon size in pixels; overrides the positional argument",
    )

    args = parser.parse_args()
    icon_size = args.size_override if args.size_override is not None else args.icon_size

    if icon_size <= 0:
        parser.error("icon size must be positive")

    return icon_size


def sanitize_name(name):
    name = name.lower()
    name = re.sub(r"[^a-z0-9_]", "_", name)
    name = re.sub(r"_+", "_", name).strip("_")
    return name


def find_local_svgs():
    if not SVG_DIR.is_dir():
        raise SystemExit("Missing local SVG cache: {}".format(SVG_DIR))

    paths = {}
    for path in sorted(SVG_DIR.glob("*.svg")):
        name = sanitize_name(path.stem)
        if not name:
            continue
        if name in paths:
            raise SystemExit("Duplicate SVG name '{}': {} and {}".format(
                name, paths[name], path))
        paths[name] = path

    print("found", len(paths), "local SVGs")
    return paths


def render_white_icon(svg_text, icon_size):
    # Material SVGs use path, rect, polygon, circle and friends. Drop the
    # invisible 24x24 background element, then force every real shape white.
    svg_text = re.sub(
        r'<(' + SHAPE_TAGS + r')\b[^>]*fill="none"[^>]*>(?:</\1>)?',
        "",
        svg_text,
    )
    png_bytes = resvg_py.svg_to_bytes(
        svg_string=svg_text,
        style_sheet="{}{{fill:#ffffff}}".format(",".join(SHAPE_TAGS.split("|"))),
        width=icon_size,
        height=icon_size,
    )
    icon = Image.open(io.BytesIO(png_bytes)).convert("RGBA")
    bbox = icon.getbbox()
    if not bbox:
        return None
    return icon.crop(bbox)


def render_svg(name, path, icon_size):
    try:
        svg = path.read_text(encoding="utf-8")
        return name, render_white_icon(svg, icon_size)
    except Exception as error:
        print("failed", name, "-", error)
        return name, None


def main(icon_size):
    svg_paths = find_local_svgs()

    missing = [name for name in REQUIRED_NAMES if name not in svg_paths]
    if missing:
        raise SystemExit("MISSING REQUIRED: " + ", ".join(missing))

    icons = []
    failures = []
    completed = 0

    with ThreadPoolExecutor(max_workers=16) as pool:
        futures = [pool.submit(render_svg, name, path, icon_size)
                   for name, path in svg_paths.items()]
        for future in as_completed(futures):
            name, icon = future.result()
            completed += 1
            if icon is None:
                failures.append(name)
            else:
                icons.append((name, icon))
            if completed % 250 == 0:
                print("processed", completed, "/", len(futures))

    if failures:
        raise SystemExit("FAILED TO RENDER: " + ", ".join(sorted(failures)))

    icons.sort(key=lambda item: item[0])
    print("rendered", len(icons), "icons at", icon_size, "px")

    cell = icon_size + PADDING * 2
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

    atlas_path = OUT_DIR / ATLAS_NAME
    atlas.save(atlas_path, format="PNG")

    manifest = {
        "image": ATLAS_NAME,
        "iconSize": icon_size,
        "iconCount": len(entries),
        "icons": entries,
    }
    with open(OUT_DIR / JSON_NAME, "w", encoding="utf-8") as f:
        json.dump(manifest, f, indent=2)

    print("wrote", atlas_path, atlas.size, "with", len(entries), "icons")


if __name__ == "__main__":
    main(parse_args())
