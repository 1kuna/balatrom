"""Built-in compatibility profiles; users cannot add their own."""
from pathlib import Path
import json
import re
from .archive import InputError, canonical_json, digest
from .conversion import symbol


REGISTRY = Path(__file__).resolve().parents[1] / "compatibility" / "profiles.json"


def recipe_dependencies(recipe):
    names = {recipe["member"]}
    if recipe["kind"] == "native-scenes":
        names.update(source["member"] for source in recipe["texts"].values())
    return names


def recipe_outputs(recipe):
    if recipe["kind"] == "native-scenes":
        return None  # Exact partition depends on derived private keys/pixels.
    names = [recipe["symbol"] + ".c"]
    if recipe["kind"] == "native-records":
        names.append(recipe["symbol"] + ".h")
    elif recipe["kind"] == "native-pairs":
        names.extend((recipe["symbol"] + "_selection.c", recipe["symbol"] + "_selection.h"))
    return names


def validate_outputs(recipe, names):
    names = set(names)
    expected = recipe_outputs(recipe)
    if expected is not None:
        if names != set(expected):
            raise InputError("Generated output contract mismatch")
        return
    prefix = re.escape(recipe["symbol"])
    pattern = r"(?:pb_direct_group_(?:[0-9]|[1-5][0-9]|6[0-2])\.c|" + prefix + r"_rows_[0-9]{1,2}\.c|" + prefix + r"_families\.c|" + prefix + r"\.h)"
    if not names or any(not re.fullmatch(pattern, name) for name in names):
        raise InputError("Unexpected dynamic scene output")
    if not {"pb_direct_group_0.c", recipe["symbol"] + "_rows_0.c", recipe["symbol"] + "_families.c", recipe["symbol"] + ".h"} <= names:
        raise InputError("Incomplete dynamic scene outputs")


def validate_scene(recipe, members):
    vector(recipe["dimensions"], 2, 1, 4096)
    vector(recipe["background"], 3, 0, 255)
    integer(recipe["bank_start"], 1, 120)
    integer(recipe["group_records"], 1, 72)
    texts = recipe["texts"]
    if not isinstance(texts, dict) or not 1 <= len(texts) <= 256:
        raise InputError("Scene text bindings are required")
    for binding, source in texts.items():
        symbol(binding)
        exact_keys(source, ("member", "assignment", "selector"))
        if not isinstance(source["member"], str) or source["member"] not in members:
            raise InputError("Scene text member is not fingerprinted")
        if source["assignment"] is not None:
            assignment = source["assignment"]
            if not isinstance(assignment, list) or not 1 <= len(assignment) <= 8 or any(
                    not isinstance(p, str) or not re.fullmatch(r"[A-Za-z_][A-Za-z_0-9]*", p) for p in assignment):
                raise InputError("Invalid scene static initializer")
        selector = source["selector"]
        if not isinstance(selector, list) or not 1 <= len(selector) <= 32 or any(type(p) not in (str, int) for p in selector):
            raise InputError("Invalid scene text selector")
    families = recipe["families"]
    if not isinstance(families, list) or not 1 <= len(families) <= 11:
        raise InputError("Scene palette families outside native contract")
    family_ids = set()
    for family in families:
        exact_keys(family, ("id", "bg_samples", "obj_samples"))
        integer(family["id"], 0, 10)
        if family["id"] in family_ids:
            raise InputError("Duplicate scene palette family")
        family_ids.add(family["id"])
        for field, count in (("bg_samples", 4), ("obj_samples", 3)):
            samples = family[field]
            if not isinstance(samples, list) or len(samples) != 8:
                raise InputError("Invalid scene palette sample rows")
            for row in samples:
                if not isinstance(row, list) or len(row) != count:
                    raise InputError("Invalid scene palette width")
                for point in row:
                    vector(point, 2, 0, 4095)
                    if point[0] >= recipe["dimensions"][0] or point[1] >= recipe["dimensions"][1]:
                        raise InputError("Scene palette sample outside source")
    views = recipe["views"]
    if not isinstance(views, list) or not 1 <= len(views) <= 128:
        raise InputError("Invalid declared scene views")
    used_bindings = set()
    row_count = 0
    for view in views:
        exact_keys(view, ("origin", "pairs", "frame"))
        vector(view["origin"], 2, 0, 4095)
        if view["origin"][0] + 160 > recipe["dimensions"][0] or view["origin"][1] + 144 > recipe["dimensions"][1]:
            raise InputError("Scene view must cover a 160x144 source region")
        pairs = view["pairs"]
        if not isinstance(pairs, list) or not 1 <= len(pairs) <= 180 or any(type(p) is not int for p in pairs) or len(set(pairs)) != len(pairs):
            raise InputError("Invalid or duplicate declared scene pairs")
        for pair in pairs:
            integer(pair, 0, 179)
        row_count += len(pairs) * len(families)
        frame = view["frame"]
        exact_keys(frame, ("target", "jokers", "glyphs", "cards"))
        if frame["target"] is not None:
            if not isinstance(frame["target"], str) or frame["target"] not in texts:
                raise InputError("Missing local target text binding")
            used_bindings.add(frame["target"])
        for field, maximum, keys in (("jokers", 5, ("x", "kind", "id")),
                                    ("glyphs", 15, ("binding", "x", "y", "font", "colour")),
                                    ("cards", 8, ("x", "y", "kind", "rank", "suit", "cues"))):
            objects = frame[field]
            if not isinstance(objects, list) or len(objects) > maximum:
                raise InputError("Invalid scene object count")
            for obj in objects:
                exact_keys(obj, keys)
                for key in keys:
                    if key == "binding":
                        if not isinstance(obj[key], str) or obj[key] not in texts:
                            raise InputError("Scene glyph needs local text binding")
                        used_bindings.add(obj[key])
                    else:
                        integer(obj[key], 0, 255)
                if field in ("jokers", "cards"):
                    integer(obj["kind"], 1, 3)
                if field == "cards":
                    integer(obj["y"], 5, 143)
                    integer(obj["rank"], 2, 14)
                    integer(obj["suit"], 0, 3)
                    integer(obj["cues"], 0, 15)
                if field == "glyphs":
                    integer(obj["font"], 0, 1)
    if used_bindings != set(texts):
        raise InputError("Every scene text binding must feed declared geometry")
    if row_count > 10000:
        raise InputError("Scene enumeration exceeds bounded coverage")


def exact_keys(value, keys):
    if not isinstance(value, dict) or set(value) != set(keys):
        raise InputError("Invalid compatibility profile fields")


def integer(value, low, high):
    if type(value) is not int or not low <= value <= high:
        raise InputError("Invalid compatibility profile integer")


def vector(value, count, low, high):
    if not isinstance(value, list) or len(value) != count:
        raise InputError("Invalid compatibility profile vector")
    for entry in value:
        integer(entry, low, high)


def validate(profile):
    exact_keys(profile, ("id", "retail_version", "manifest_sha256", "members", "recipes", "engine"))
    if not isinstance(profile["id"], str) or not re.fullmatch(r"[a-z0-9][a-z0-9-]{0,63}", profile["id"]):
        raise InputError("Invalid profile ID")
    if not isinstance(profile["retail_version"], str) or not profile["retail_version"]:
        raise InputError("Profile has no verified retail version")
    if not re.fullmatch(r"[0-9a-f]{64}", str(profile["manifest_sha256"])):
        raise InputError("Invalid complete member manifest fingerprint")
    members = profile["members"]
    if not isinstance(members, dict) or not members or len(members) > 10000:
        raise InputError("Profile needs required member fingerprints")
    for name, metadata in members.items():
        if not isinstance(name, str) or not name or name.startswith("/") or any(
                p in ("", ".", "..") for p in name.split("/")) or "\\" in name or ":" in name:
            raise InputError("Unsafe profile member")
        exact_keys(metadata, ("bytes", "sha256"))
        integer(metadata["bytes"], 1, 64 * 1024 * 1024)
        if not re.fullmatch(r"[0-9a-f]{64}", str(metadata["sha256"])):
            raise InputError("Invalid member fingerprint")
    recipes = profile["recipes"]
    if not isinstance(recipes, list) or not recipes or len(recipes) > 2048:
        raise InputError("Profile needs bounded conversion recipes")
    outputs = set()
    sources = set()
    for recipe in recipes:
        if not isinstance(recipe, dict) or recipe.get("kind") not in ("png-tiles", "lua-text", "lua-assignment-text", "native-records", "native-pairs", "native-scenes"):
            raise InputError("Unknown conversion kind")
        base = {"kind", "member", "symbol"}
        extra = {"png-tiles": {"dimensions", "crop", "background", "max_tiles"},
                 "native-records": {"dimensions", "crops", "background", "bank", "resample", "tile_symbol", "palette_symbol"},
                 "native-pairs": {"dimensions", "crops", "background", "bank", "resample", "bg_samples", "obj_samples", "keys", "group"},
                 "native-scenes": {"dimensions", "background", "bank_start", "group_records", "families", "texts", "views"},
                 "lua-text": {"selectors", "max_bytes"},
                 "lua-assignment-text": {"selectors", "max_bytes", "assignment"}}[recipe["kind"]]
        exact_keys(recipe, base | extra)
        name = symbol(recipe["symbol"])
        # Reserve all emitted globals, not just the recipe base.
        emitted = ({name + suffix for suffix in ("_tiles", "_map", "_palette")}
                   if recipe["kind"] == "png-tiles" else
                   {symbol(recipe["tile_symbol"]), symbol(recipe["palette_symbol"])} if recipe["kind"] == "native-records" else
                   {"pb_direct_group_" + str(recipe["group"]), "pb_direct_read_" + str(recipe["group"]),
                    name + "_selection", name + "_colors", name + "_bg_ids", name + "_obj_ids"}
                   if recipe["kind"] == "native-pairs" else {name})
        if emitted & outputs:
            raise InputError("Generated C symbols collide")
        if recipe["kind"] == "native-records" and recipe["tile_symbol"] == recipe["palette_symbol"]:
            raise InputError("Native tile and palette symbols must differ")
        outputs.update(emitted)
        if recipe["member"] not in members:
            raise InputError("Recipe member has no required fingerprint")
        sources.update(recipe_dependencies(recipe))
        if recipe["kind"] == "native-scenes":
            validate_scene(recipe, members)
            continue
        if recipe["kind"] in ("png-tiles", "native-records", "native-pairs"):
            vector(recipe["dimensions"], 2, 1, 4096)
            vector(recipe["background"], 3, 0, 255)
            if recipe["kind"] == "png-tiles":
                integer(recipe["max_tiles"], 1, 256)
                crops = [recipe["crop"]]
            else:
                integer(recipe["bank"], 1, 255)
                if recipe["resample"] != "nearest":
                    raise InputError("Native resampling method is unsupported")
                crops = recipe["crops"]
                maximum = 147 if recipe["kind"] == "native-records" else 512
                if not isinstance(crops, list) or not 1 <= len(crops) <= maximum:
                    raise InputError("Native records exceed data-bank budget")
            for crop in crops:
                vector(crop, 4, 0, 4096)
                x, y, w, h = crop
                if not w or not h or x + w > recipe["dimensions"][0] or y + h > recipe["dimensions"][1]:
                    raise InputError("Invalid native crop")
                if recipe["kind"] == "png-tiles" and (w % 8 or h % 8):
                    raise InputError("Tile crop must be aligned to eight pixels")
            if recipe["kind"] == "native-pairs":
                integer(recipe["group"], 0, 62)
                if name != "pb_direct_group_" + str(recipe["group"]):
                    raise InputError("Pair filename must match native group ABI")
                keys = recipe["keys"]
                if not isinstance(keys, list) or len(keys) != len(crops):
                    raise InputError("Pair crops require an exact semantic key list")
                for key in keys:
                    if not isinstance(key, list) or not 1 <= len(key) <= 119:
                        raise InputError("Invalid native pair key")
                    for value in key:
                        integer(value, 0, 255)
                if len({bytes(k) for k in keys}) != len(keys):
                    raise InputError("Duplicate semantic pair key")
                for field, count in (("bg_samples", 4), ("obj_samples", 3)):
                    samples = recipe[field]
                    if not isinstance(samples, list) or len(samples) != 8:
                        raise InputError("Invalid native palette sample rows")
                    for row in samples:
                        if not isinstance(row, list) or len(row) != count:
                            raise InputError("Invalid native palette sample width")
                        for point in row:
                            vector(point, 2, 0, 4095)
                            if point[0] >= recipe["dimensions"][0] or point[1] >= recipe["dimensions"][1]:
                                raise InputError("Palette sample is outside PNG")
        else:
            if recipe["kind"] == "lua-assignment-text":
                assignment = recipe["assignment"]
                if not isinstance(assignment, list) or not 1 <= len(assignment) <= 8 or any(
                        not isinstance(p, str) or not re.fullmatch(r"[A-Za-z_][A-Za-z_0-9]*", p) for p in assignment):
                    raise InputError("Invalid static assignment selector")
            integer(recipe["max_bytes"], 1, 4096)
            paths = recipe["selectors"]
            if not isinstance(paths, list) or not 1 <= len(paths) <= 4096:
                raise InputError("Invalid text selectors")
            for path in paths:
                if not isinstance(path, list) or not 1 <= len(path) <= 32 or any(
                        type(k) not in (str, int) for k in path):
                    raise InputError("Invalid literal-table selector")
    if sources != set(members):
        raise InputError("Every required member must feed actual conversion")
    scenes = [r for r in recipes if r["kind"] == "native-scenes"]
    if len(scenes) > 1 or scenes and any(r["kind"] == "native-pairs" for r in recipes):
        raise InputError("Only one scene atlas owns direct-cache group namespace")
    # A profile can enable conversion before an engine exists for it.
    if profile["engine"] is not None:
        raise InputError("Engine integration is not supported here")
    return profile


def registry():
    data = json.loads(REGISTRY.read_text())
    exact_keys(data, ("schema", "profiles"))
    if data["schema"] != 1 or not isinstance(data["profiles"], list):
        raise InputError("Unsupported profile registry")
    profiles = [validate(p) for p in data["profiles"]]
    if len({p["id"] for p in profiles}) != len(profiles):
        raise InputError("Duplicate registered profile ID")
    return profiles


def match(fingerprint, profiles=None):
    exact_keys(fingerprint, ("archive_sha256", "manifest_sha256", "members"))
    for key in ("archive_sha256", "manifest_sha256"):
        if not re.fullmatch(r"[0-9a-f]{64}", str(fingerprint[key])):
            raise InputError("Invalid input fingerprint")
    rows = fingerprint["members"]
    if not isinstance(rows, list) or not 1 <= len(rows) <= 10000:
        raise InputError("Invalid fingerprint member list")
    names = set()
    for row in rows:
        exact_keys(row, ("name", "bytes", "sha256"))
        if not isinstance(row["name"], str) or row["name"] in names:
            raise InputError("Invalid or duplicate fingerprint member")
        names.add(row["name"])
        integer(row["bytes"], 0, 64 * 1024 * 1024)
        if not re.fullmatch(r"[0-9a-f]{64}", str(row["sha256"])):
            raise InputError("Invalid fingerprint member hash")
    if rows != sorted(rows, key=lambda r: r["name"]) or digest(canonical_json(rows)) != fingerprint["manifest_sha256"]:
        raise InputError("Input member manifest is inconsistent")
    matches = [p for p in (registry() if profiles is None else profiles)
               if validate(p)["manifest_sha256"] == fingerprint["manifest_sha256"]]
    if len(matches) != 1:
        raise InputError("No single supported game version matches; conversion stopped")
    profile = matches[0]
    actual = {m["name"]: {"bytes": m["bytes"], "sha256": m["sha256"]} for m in rows}
    if any(actual.get(name) != metadata for name, metadata in profile["members"].items()):
        raise InputError("Required retail member fingerprint mismatch")
    return profile


def profile_hash(profile):
    return digest(canonical_json(validate(profile)))
