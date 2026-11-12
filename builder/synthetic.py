"""Original synthetic end-to-end fixture. Not a game or retail profile.

Create geometric pixels and literal test strings, import through the same
converter, then build a separate authored ABI consumer with an explicit
5/197-unit manifest. Never register this profile in production compatibility.
"""
from io import BytesIO
from pathlib import Path
import json
import re
import zipfile
from .archive import Archive, InputError, digest
from .build_contract import compile_contract, validate_contract
from .pipeline import import_local, verify_workspace
from .profiles import validate


def fixture_profile(path):
    with Archive(path) as archive:
        fingerprint = archive.fingerprint()
    members = {m["name"]: {"bytes": m["bytes"], "sha256": m["sha256"]} for m in fingerprint["members"]}
    families = []
    for identifier in (0, 1):
        bg = [[i, 0] for i in (range(4) if identifier == 0 else reversed(range(4))) ]
        obj = [[i, 0] for i in (range(4, 7) if identifier == 0 else reversed(range(4, 7))) ]
        families.append({"id": identifier, "bg_samples": [bg[:] for _ in range(8)], "obj_samples": [obj[:] for _ in range(8)]})
    views = []
    for index, binding in enumerate(("first", "second")):
        views.append({"origin": [index * 160, 0], "pairs": [0, 5, 105, 125], "frame": {
            "target": "target", "jokers": [{"x": 40, "kind": 1, "id": index}],
            "glyphs": [{"binding": binding, "x": 40, "y": 90, "font": 0, "colour": 1}],
            "cards": [{"x": 40, "y": 100, "kind": 1, "rank": 3 + index, "suit": index, "cues": 0}]}})
    recipes = [{"kind": "native-records", "member": "pixels.png", "symbol": "sidebar_test",
                "tile_symbol": "sidebar_test_tiles", "palette_symbol": "sidebar_test_palettes",
                "dimensions": [320, 144], "crops": [[0, 0, 16, 24]], "background": [0, 0, 0],
                "bank": 17, "resample": "nearest"},
               {"kind": "native-scenes", "member": "pixels.png", "symbol": "scene_test",
                "dimensions": [320, 144], "background": [0, 0, 0], "bank_start": 20,
                "group_records": 72, "families": families, "views": views,
                "texts": {name: {"member": "text.lua", "assignment": None, "selector": [name]}
                          for name in ("first", "second", "target")}}]
    return validate({"id": "synthetic-e2e-not-retail", "retail_version": "NOT_A_RETAIL_VERSION",
                     "manifest_sha256": fingerprint["manifest_sha256"], "members": members,
                     "recipes": recipes, "engine": None})


def create_input(path):
    from PIL import Image
    colors = [(0, 0, 0), (248, 0, 0), (0, 248, 0), (0, 0, 248), (248, 248, 0), (0, 248, 248), (248, 0, 248)]
    im = Image.new("RGB", (320, 144))
    im.putdata([colors[((x % 160) + y * 3) % 7] for y in range(144) for x in range(320)])
    buf = BytesIO()
    im.save(buf, format="PNG")
    with zipfile.ZipFile(path, "w") as archive:
        archive.writestr("pixels.png", buf.getvalue())
        archive.writestr("text.lua", b"return {first='PAIR',second='FLUSH',target='123456'}")
    path.chmod(0o600)


def prepare_fixture(root, unit_count=197):
    if unit_count not in (5, 197):
        raise InputError("Synthetic fixture supports exactly 5 or 197 ordered units")
    root = Path(root)
    if root.exists() or root.is_symlink():
        raise InputError("Synthetic fixture requires a new directory")
    root.mkdir(mode=0o700)
    try:
        game = root / "synthetic.love"
        create_input(game)
        profile = fixture_profile(game)
        receipt = import_local(game, root / "private-content", profiles=[profile])
        verify_workspace(root / "private-content", profiles=[profile])
        sources = root / "separated-sources"
        (sources / "engine").mkdir(parents=True, mode=0o700)
        (sources / "content").mkdir(mode=0o700)
        (sources / "test-data").mkdir(mode=0o700)
        entries, objects = [], []
        atlas = next(row["details"] for row in receipt["generated"] if row["conversion"]["kind"] == "native-scenes")
        for row in receipt["generated"]:
            name = Path(row["path"]).name
            data = (root / "private-content" / row["path"]).read_bytes()
            target = "content/" + name
            (sources / target).write_bytes(data)
            entries.append({"path": target, "sha256": digest(data), "role": "derived-content", "input_members": row["input_dependencies"]})
            if name.endswith(".c"):
                bank = atlas["source_banks"][name] if row["conversion"]["kind"] == "native-scenes" else row["conversion"]["bank"]
                objects.append({"id": Path(name).stem, "source": target, "bank": bank})
        # Hash-only oracle metadata is derived privately, not a plane fallback.
        sidebar_source = (root / "private-content/generated/sidebar_test.c").read_text()
        sidebar_sum = sum(int(v) for array in ("sidebar_test_tiles", "sidebar_test_palettes")
                          for v in re.search(array + r"\[\d+\]=\{([^}]*)\}", sidebar_source)[1].split(",")) & 65535
        oracle = "#include <stdint.h>\nstatic const uint16_t expected_sidebar_sum=%du;\nstatic const uint16_t expected_checksums[%d]={%s};\n" % (
            sidebar_sum,
            len(atlas["record_checksums"]), ",".join(map(str, atlas["record_checksums"])))
        family_source = (root / "private-content/generated/scene_test_families.c").read_text()
        family_sum = sum(int(v) for array in ("scene_test_families", "scene_test_family_ids", "scene_test_colors")
                         for v in re.search(array + r"\[\d+\]=\{([^}]*)\}", family_source)[1].split(",")) & 65535
        color_count = len(re.search(r"scene_test_colors\[\d+\]=\{([^}]*)\}", family_source)[1].split(","))
        oracle += "static const uint16_t expected_family_sum=%du;\nstatic const uint32_t expected_row_keys[%d]={%s};\n" % (
            family_sum, len(atlas["row_codes"]), ",".join(str(v) + "ul" for v in atlas["row_codes"]))
        oracle_path = "content/oracle.h"
        (sources / oracle_path).write_text(oracle)
        entries.append({"path": oracle_path, "sha256": digest(oracle.encode()), "role": "derived-content",
                        "input_members": {name: member["sha256"] for name, member in profile["members"].items()}})
        marker_count = unit_count - len(objects) - 1
        marker_declarations, marker_rows = [], []
        for i in range(marker_count):
            name = "synthetic_marker_%03d" % i
            code = "#pragma bank 255\n#include <gb/gb.h>\nBANKREF(%s)\nconst uint8_t %s[1]={%d};\n" % (name, name, i & 255)
            target = "test-data/" + name + ".c"
            (sources / target).write_text(code)
            entries.append({"path": target, "sha256": digest(code.encode()), "role": "synthetic-test-data", "input_members": {}})
            objects.append({"id": name, "source": target, "bank": 1 + i % 83})
            marker_declarations.append("extern const uint8_t %s[1];BANKREF_EXTERN(%s)" % (name, name))
            marker_rows.append("{%s,%d,%d}" % (name, 1 + i % 83, i & 255))
        if len(atlas["groups"]) != 1 or len(atlas["tables"]) != 1:
            raise InputError("Synthetic consumer template requires one generated group/table")
        engine = """#include <gb/gb.h>
#include <stdint.h>
#include "scene_test.h"
#include "sidebar_test.h"
#include "oracle.h"
uint8_t raster_pixels[128],raster_planes[96];
""" + "\n".join(marker_declarations)
        if marker_count:
            engine += "\ntypedef struct {const uint8_t *data;uint8_t bank,value;} TestMarker;\nstatic const TestMarker markers[%d]={%s};\n" % (marker_count, ",".join(marker_rows))
        engine += """
void main(void){
 volatile uint8_t *status=(volatile uint8_t*)0xd7f0;
 uint16_t row,handle,recipe,sum;uint8_t i;
 *status=0;
 SWITCH_ROM(BANK(sidebar_test_tiles));
 sum=0;for(i=0;i<96u;i++)sum+=sidebar_test_tiles[i];
 for(i=0;i<4u;i++)sum+=sidebar_test_palettes[i];
 if(sum!=expected_sidebar_sum){*status=1;return;}
"""
        if marker_count:
            engine += "for(row=0;row<%du;row++){SWITCH_ROM(markers[row].bank);if(markers[row].data[0]!=markers[row].value){*status=2;return;}}\n" % marker_count
        engine += """
 for(row=0;row<%du;row++){
  SWITCH_ROM(BANK(scene_test_rows_0));handle=scene_test_rows_0[row];
  if(scene_test_rows_0_keys[row]!=expected_row_keys[row]){*status=6;return;}
  if((handle>>10)!=0u || !pb_direct_read_0(handle&1023u,&recipe)){*status=3;return;}
  sum=(recipe&255u)+(recipe>>8);
  for(i=0;i<96u;i++)sum+=raster_planes[i];
  if(sum!=expected_checksums[row]){*status=4;return;}
 }
 SWITCH_ROM(BANK(scene_test_families));
 if(scene_test_family_ids[0]!=0u || scene_test_family_ids[1]!=1u){*status=5;return;}
 sum=0;for(i=0;i<112u;i++)sum+=scene_test_families[i];
 for(i=0;i<2u;i++)sum+=scene_test_family_ids[i];
 for(i=0;i<%du;i++)sum+=scene_test_colors[i];
 if(sum!=expected_family_sum){*status=7;return;}
 *status=0x7f;
 while(1){vsync();}
}
""" % (atlas["declared_rows"], color_count)
        engine_path = "engine/main.c"
        (sources / engine_path).write_text(engine)
        entries.append({"path": engine_path, "sha256": digest(engine.encode()), "role": "authored-engine", "input_members": {}})
        objects.insert(0, {"id": "synthetic_engine", "source": engine_path, "bank": 0})
        contract = validate_contract({"schema": 1, "purpose": "synthetic-abi", "sources": entries,
                                      "objects": objects, "include_dirs": ["content"],
                                      "rom": {"title": "SYNTHABI", "banks": 128},
                                      "budgets": {"fixed_reserve": 1024, "stack_reserve": 1024}})
        (root / "SYNTHETIC_PROFILE_NOT_RETAIL.json").write_text(json.dumps(profile, indent=2) + "\n")
        (root / "CONTRACT.json").write_text(json.dumps(contract, indent=2) + "\n")
        (root / ".gitignore").write_text("*\n")
        return profile, contract, atlas
    except Exception:
        import shutil
        shutil.rmtree(root)
        raise


def run_fixture(root, compiler, unit_count=197, emulate=False, progress=None):
    root = Path(root)
    profile, contract, atlas = prepare_fixture(root, unit_count)
    report = compile_contract(root / "separated-sources", contract, root / "linked", compiler, progress)
    execution = {"performed": False, "faithful_game_validated": False}
    if emulate:
        try:
            from pyboy import PyBoy
        except ImportError as exc:
            raise InputError("Optional synthetic execution requires an already installed PyBoy") from exc
        emulator = PyBoy(str(root / "linked/synthetic-abi-check.gbc"), window="null", sound_emulated=False)
        try:
            for frames in range(120):
                emulator.tick(1, False)
                status = emulator.memory[0xd7f0]
                if status:
                    break
            if status != 0x7f:
                raise InputError("Synthetic ABI consumer did not pass: status=" + str(status))
            execution.update(performed=True, status=status, frames=frames + 1)
        finally:
            emulator.stop(save=False)
    summary = {"kind": "synthetic-end-to-end-not-a-game", "ordered_units": len(contract["objects"]),
               "declared_scene_rows": atlas["declared_rows"], "unique_scene_keys": atlas["unique_keys"],
               "palette_families": len(atlas["family_ids"]), "source_roles": sorted({s["role"] for s in contract["sources"]}),
               "rom": report["rom"], "emulation": execution, "verified_retail_profiles": 0,
               "faithful_game_validated": False, "canonical_sources_used": False, "hardware_used": False}
    (root / "SYNTHETIC_RESULT.json").write_text(json.dumps(summary, indent=2) + "\n")
    return summary
