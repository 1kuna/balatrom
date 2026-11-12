"""Build the ROM offline from a supported install. Never runs the game, uses the network or flashes a cartridge."""
from pathlib import Path
import hashlib
import json
import os
import re
import shutil
import signal
import subprocess
import sys
import time
import threading
from concurrent.futures import ThreadPoolExecutor, as_completed

from .archive import Archive, InputError
from .input_identity import matches
from .build_progress import BuildProgress, current as current_progress, phase
from .platform_process import ManagedCompiler
from .build_contract import object_areas, validate_rom

ARCHIVE_SHA = "48c7a0791796a969d2cd0891ebdc9922b2988eb5aaad8ad7a72775a02772e24e"
MANIFEST_SHA = "ae73502735ee949d8c74243469dee48663e4f52d4c6f9bdd61f81419d4cd5a2c"
REGISTRY = Path(__file__).resolve().parents[1] / "compatibility" / "owned_profiles.json"


def sha(path):
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()


def write_json(path, value):
    Path(path).write_text(json.dumps(value, indent=2) + "\n")


def profile_rom_target(profile, manifest):
    """Both registered profile and packet must pin the same exact build."""
    expected = profile.get("expected_ROM_sha256")
    if not isinstance(expected, str) or not re.fullmatch(r"[0-9a-f]{64}", expected):
        raise InputError("Private profile lacks an exact ROM target")
    if manifest.get("expected_reference_ROM_sha256") != expected:
        raise InputError("Engine packet targets a different registered build")
    return expected


def private_destination(output, game_path, profile_root=None):
    destination = Path(output).expanduser().absolute()
    if destination.exists() or destination.is_symlink():
        raise InputError("Output must be a new private directory; no overwrite/cache fallback")
    parent = destination.parent.resolve(strict=True)
    if not parent.is_dir():
        raise InputError("Output parent must already exist")
    destination = parent / destination.name
    install_root = Path(game_path).expanduser().resolve(strict=True).parent
    profile_root = (profile_root or (Path(os.environ.get("APPDATA", str(Path.home() / "AppData" / "Roaming"))) / "Balatro" if os.name == "nt" else Path.home() / "Library" / "Application Support" / "Balatro")).resolve()
    for protected in (install_root, profile_root):
        if destination == protected or destination.is_relative_to(protected):
            raise InputError("Private output must be outside the game installation and profile/save directory")
    return destination


def build_owned(game_path, engine_packet, output, lcc, assemble_only=False, preflight=False, cancel_check=lambda: None, *, compile_workers=1, object_cache=None):
    if preflight:
        return _build_owned_impl(game_path,engine_packet,output,lcc,assemble_only,preflight,cancel_check,compile_workers=compile_workers,object_cache=object_cache)
    destination=private_destination(output,game_path)
    with BuildProgress(destination.parent/('BUILD-STAGES-'+destination.name+'.json')):
        return _build_owned_impl(game_path,engine_packet,output,lcc,assemble_only,preflight,cancel_check,compile_workers=compile_workers,object_cache=object_cache)


def _build_owned_impl(game_path, engine_packet, output, lcc, assemble_only=False, preflight=False, cancel_check=lambda: None, *, compile_workers=1, object_cache=None):
    if sys.version_info < (3, 12):
        raise InputError("Python3.12+ is required")
    try:
        import PIL
    except ImportError as exc:
        raise InputError("Pillow12.3.0 is missing; in the Python3.12+ environment used for this source CLI, run: python -m pip install --only-binary=:all: Pillow==12.3.0") from exc
    if PIL.__version__ != "12.3.0":
        raise InputError("Use tested Pillow12.3.0; the builder installs nothing")
    from .owned_recipes.packet import load, assemble
    from .owned_recipes.recipes import derive
    from .owned_recipes.legacy_cards import derive as legacy_cards
    from .owned_recipes.catalog_scalars import derive as catalog_scalars
    from .owned_recipes.card_rails import transform

    packet = Path(engine_packet).expanduser().absolute()
    destination = private_destination(output, game_path)
    compiler = Path(lcc).expanduser().absolute()
    if packet.is_symlink() or not packet.is_dir():
        raise InputError("Supply an explicit regular private engine packet")
    free_bytes = shutil.disk_usage(destination.parent).free
    if free_bytes < 2 * 1024**3:
        raise InputError("At least 2GiB free space required before starting; choose another output volume or free space")
    phase("verify","Verifying source and tools")
    profiles = json.loads(REGISTRY.read_text())["profiles"]
    profile = next((p for p in profiles if p["engine_manifest_sha256"] == sha(packet / "manifest.json")), None)
    if profile is None:
        raise InputError("Engine directory does not match a registered build profile")
    manifest = load(packet)
    omit_hybrid = manifest.get("unused_hybrid_omitted") == 1
    source_count = manifest["descriptor"]["source_inputs"]
    if source_count not in (487, 488):
        raise InputError("Unsupported source input count")
    if (len(manifest["rows"]) != source_count or
            len(manifest["descriptor"]["objects"]) != 198 or
            manifest["descriptor"]["source_inputs"] != source_count or
            (omit_hybrid and any(r["path"] == "generated/hybrid_cases.inc" for r in manifest["rows"]))):
        raise InputError("Incomplete engine packet")
    expected_rom = profile_rom_target(profile, manifest)
    if os.name == "nt":
        if compiler.name.lower() != "lcc.exe" or not compiler.is_file():
            raise InputError("Supply the pinned GBDK4.5.0 win64 bin/lcc.exe")
        sdk_record = json.loads((Path(__file__).with_name("windows_sdk.json")).read_text())
        prerequisites = sdk_record["prerequisites"]
    else:
        if compiler.name != "lcc" or not compiler.is_file() or not os.access(compiler, os.X_OK):
            raise InputError("Supply the existing tested GBDK bin/lcc")
        prerequisites = manifest["toolchain"]
    sdk = compiler.parent.parent
    for prerequisite in prerequisites:
        relative = Path(prerequisite["path"])
        if relative.is_absolute() or ".." in relative.parts:
            raise InputError("Invalid SDK-relative prerequisite")
        if not (sdk / relative).is_file() or (sdk / relative).is_symlink() or sha(sdk / relative) != prerequisite["sha256"]:
            raise InputError("Unsupported SDK prerequisite: " + str(relative))
    cancel_check()
    started = time.monotonic()
    phase("verify","Verifying Balatro")
    with Archive(game_path) as archive:
        fingerprint = archive.fingerprint()
        if not matches({k:fingerprint[k] for k in ("archive_sha256","manifest_sha256")}, {"archive_sha256":ARCHIVE_SHA,"manifest_sha256":MANIFEST_SHA}):
            raise InputError("Unsupported installation: the game files must match the supported Balatro version exactly")
        if preflight:
            archive.assert_unchanged()
            return {"profile_id": profile["id"], "preflight_passed": True,
                    "scope": "prerequisites checked; nothing reconstructed or compiled",
                    "source_inputs": source_count, "compiler_units": 198,
                    "expected_ROM_sha256": expected_rom, "free_bytes": free_bytes,
                    "output_created": False, "ROM_built": False,
                    "public_export_admitted": False}
        phase("prepare","Preparing game assets")
        files, arrays, members = derive(archive, manifest["descriptor"])
        if manifest.get("native_atlas_schema_recipe") == 1:
            from .owned_recipes.atlas_schema import derive as atlas_schema
            files.update(atlas_schema())
        if manifest.get("native_routing_registry_recipe") == 1:
            from .owned_recipes.routing_registry import derive as routing_registry
            atlas_source = (packet / "source/src/pb_atlas.c").read_text()
            routing_files, _ = routing_registry(atlas_source, manifest["descriptor"])
            files.update(routing_files)
        if manifest.get("native_plain_template_recipe") == 1:
            from .owned_recipes.plain_templates import derive as plain_templates
            files.update(plain_templates(packet))
        if manifest.get("native_semantic_palette_recipe") == 1:
            from .owned_recipes.semantic_palette import derive as semantic_palette
            files.update(semantic_palette())
        phase("prepare","Preparing cards")
        legacy, legacy_receipt, _ = legacy_cards(archive)
        arrays, legacy = transform(arrays, legacy)
        files["generated/owner_legacy_cards.inc"] = legacy.encode()
        phase("prepare","Preparing game data")
        catalog, _ = catalog_scalars(archive.read("game.lua"))
        arrays.update(catalog)
        if manifest.get("functional_metadata_recipe") == 1:
            from .owned_recipes.runtime_metadata import derive as runtime_metadata
            metadata = runtime_metadata(archive.read("game.lua"))
            id_header = (packet / "source/include/joker_ids.h").read_text()
            pairs = re.findall(r"#define (J_\w+) (\d+)", id_header)
            ids = {int(value): name for name, value in pairs}
            if len(pairs) != 150 or set(ids) != set(range(150)):
                raise InputError("Native catalog ID boundary changed")
            metadata["blueprint_cases"] = [ids[i] for i, compatible in enumerate(metadata["blueprint_compat"]) if not compatible]
            arrays.update(metadata)
        phase("prepare","Verifying reconstructed sources")
        source = assemble(packet, manifest, files, arrays)
        archive.assert_unchanged()
        destination.mkdir(mode=0o700)
        (destination / ".gitignore").write_text("*\n")
        view = destination / "view"
        view.mkdir(mode=0o700)
        for relative, data in source.items():
            path = view / relative
            path.parent.mkdir(parents=True, exist_ok=True)
            path.write_bytes(data)
        receipt = {"profile_id": profile["id"], "scope": "local build from your own install; not the full game",
                   "archive_sha256": ARCHIVE_SHA, "archive_manifest_sha256": MANIFEST_SHA,
                   "input_member_sha256": members, "engine_packet_manifest_sha256": sha(packet / "manifest.json"),
                   "source_sha256": {name: hashlib.sha256(data).hexdigest() for name, data in source.items()},
                   "legacy_card_conversion": legacy_receipt, "retail_Lua_executed": False,
                   "game_launched": False, "network_access": False, "user_profiles_or_saves_accessed": False,
                   "cached_game_objects": 0, "raw_archive_members_copied": False, "ROM_built": False,
                   "public_export_admitted": False}
        write_json(destination / "OWNED_INSTALL_RECEIPT.json", receipt)
        if assemble_only:
            return receipt
        cancel_check()
        result = compile_fresh(destination, manifest["descriptor"], compiler, cancel_check=cancel_check, workers=compile_workers, cache_dir=object_cache)
        if result["rom"]["sha256"] != expected_rom:
            raise InputError("Fresh ROM differs from the registered build; output rejected")
        archive.assert_unchanged()
        receipt.update(ROM_built=True, ROM_sha256=expected_rom, fresh_compiler_units=result["fresh_compiler_units"], cached_game_objects=result["cached_game_objects"],
                       wall_seconds=time.monotonic() - started)
        write_json(destination / "OWNED_INSTALL_RECEIPT.json", receipt)
        return receipt


def _scheduled_entries(entries):
    # Measured slow units from the registered 198-unit profile. Starting these
    # early reduces the tail; this never changes bank assignment or link order.
    priority = {name: index for index, name in enumerate((
        "a1_font_3", "pb_direct_group_24", "scoring", "raster_prepare", "creation_service", "render"))}
    return sorted(entries, key=lambda entry: priority.get(entry["name"], len(priority)))


def compile_fresh(output, descriptor, compiler, cancel_check=lambda: None, *, workers=1, cache_dir=None):
    if type(workers) is not int or not 1 <= workers <= 4:
        raise InputError("Compiler workers must be between one and four")
    output = Path(output).absolute()
    view = output / "view"
    progress = current_progress()
    phase("prepare","Checking compiler cache")
    cache = None
    if cache_dir is not None:
        from .compile_cache import ObjectCache, context_key
        try:
            cache = ObjectCache(cache_dir, context_key(view, Path(compiler).parent.parent, descriptor))
        except (OSError, ValueError) as exc:
            raise InputError("Compiler cache context could not be verified") from exc
    stop = threading.Event()
    def check():
        if stop.is_set():
            raise InputError("Compilation stopped after another unit failed")
        cancel_check()
    linked = output / "linked"
    linked.mkdir()
    objects = linked / "obj"
    objects.mkdir()
    flags = descriptor["compile_flags"]
    commands, compiled = [], []
    timings = {}
    build_started = time.monotonic()

    def relative_operand(path):
        # lcc re-spawns downstream tools on Windows without quoting bare operands.
        # Our pinned source graph uses safe relative names; all generated outputs
        # are siblings of view, so no user/root directory enters downstream argv.
        value=Path(os.path.relpath(path,view)).as_posix()
        if any(c.isspace() for c in value):
            raise InputError("Internal compiler operand contains whitespace")
        return value

    def run(arguments, label):
        command = [str(compiler), *arguments]
        commands.append(command)
        began = time.monotonic()
        check()
        process = None
        try:
            temporary=linked / "tmp" / label
            temporary.mkdir(parents=True,exist_ok=False)
            environment=dict(os.environ)
            environment.update({key:relative_operand(temporary) for key in ("TMP","TEMP","TMPDIR")})
            process = ManagedCompiler(command, cwd=view, env=environment)
            if progress:progress.unit_started(label)
            deadline = time.monotonic() + 300
            while True:
                try:
                    check()
                except BaseException:
                    try:
                        process.terminate_tree()
                    except ProcessLookupError:
                        pass  # The compiler exited between cancellation and termination.
                    process.communicate()
                    raise
                if time.monotonic() >= deadline:
                    raise subprocess.TimeoutExpired(command, 300)
                try:
                    stdout, stderr = process.communicate(timeout=0.2)
                    break
                except subprocess.TimeoutExpired:
                    continue
        except subprocess.TimeoutExpired as exc:
            try:
                process.terminate_tree()
            except ProcessLookupError:
                pass  # Preserve the original timeout when the process just exited.
            stdout, stderr = process.communicate()
            (linked / (label + ".log")).write_text(stdout + stderr)
            raise InputError("Compiler timed out: " + label) from exc
        finally:
            if process is not None:
                if process.returncode is None:
                    try:
                        process.terminate_tree()
                    except ProcessLookupError:
                        pass
                    process.communicate()
                process.close()
            if progress:progress.unit_finished(label)
        (linked / (label + ".log")).write_text(stdout + stderr)
        timings[label] = time.monotonic() - began
        if process.returncode:
            raise InputError("Compiler rejected " + label + " (exit " + str(process.returncode) + "): " + (stdout + stderr)[-1800:])

    entries = descriptor["objects"]
    if len({entry["name"] for entry in entries}) != len(entries):
        raise InputError("Duplicate compiler unit name")
    def compile_one(entry):
        name, bank = entry["name"], entry["bank"]
        if not re.fullmatch(r"[A-Za-z_][A-Za-z_0-9]*", name):
            raise InputError("Invalid unit name")
        raw = cache.get(name) if cache else None
        cached = raw is not None
        if raw is None:
            run([*flags, "-c", "-o", relative_operand(objects / (name + ".o")), entry["source"]], name)
            raw = (objects / (name + ".o")).read_bytes()
        else:
            (objects / (name + ".o")).write_bytes(raw)
            timings[name] = 0.0
        text, areas = object_areas(raw)
        if not cached and cache:
            cache.put(name, raw)
        if bank is not None:
            text = re.sub(r"(?m)^A _CODE_255 ", "A _CODE_" + str(bank) + " ", text)
            text = re.sub(r"(?m)^(S (?:b_|___bank_)\S+ Def)000000FF$", lambda m: m[1] + f"{bank:08X}", text)
        assigned = objects / (name + ".rel")
        assigned.write_text(text)
        return {**entry, "source_sha256": sha(view / entry["source"]), "object_sha256": sha(assigned),
                "cached": cached, "compile_wall_seconds": timings[name]}

    # Bound submitted work; a failure signals all running compiler groups before
    # waiting for shutdown. Later units cannot keep running after cancellation.
    phase("compile","Compiling")
    pool = ThreadPoolExecutor(max_workers=workers)
    futures = []
    completed = {}
    try:
        futures = [pool.submit(compile_one, entry) for entry in _scheduled_entries(entries)]
        for future in as_completed(futures):
            record = future.result()
            completed[record["name"]] = record
            compiled = [completed[e["name"]] for e in entries if e["name"] in completed]
            cached_count = sum(e["cached"] for e in compiled)
            write_json(output / "COMPILE_PROGRESS.json", {"fresh_units": compiled,
                       "cached_objects": cached_count, "completed_units": len(compiled), "workers": workers})
            print(f"Compiled {len(compiled)}/198 ({cached_count} reused)", flush=True)
            if progress:progress.units_completed(len(compiled))
    except BaseException:
        stop.set()
        for future in futures:
            future.cancel()
        raise
    finally:
        pool.shutdown(wait=True, cancel_futures=True)
    if len(compiled) != 198:
        raise InputError("Incomplete fresh compilation")
    object_list = linked / "objects.lk"
    object_list.write_text("".join(relative_operand(objects / (e["name"] + ".rel")) + "\n" for e in descriptor["objects"]))
    rom = linked / "balatrom.gbc"
    link_flags = [f.replace("<objects.lk>", relative_operand(object_list)).replace("<ROM-output>", relative_operand(rom)) for f in descriptor["link_flags"]]
    phase("link","Linking ROM")
    run(link_flags, "link")
    phase("verify","Verifying ROM")
    symbols = {n: int(v, 16) for n, v in re.findall(r"DEF (\S+) 0x([0-9A-Fa-f]+)", rom.with_suffix(".noi").read_text())}
    banks = {n: v for n, v in symbols.items() if n.startswith("l__CODE_")}
    fixed_end = symbols["s__CODE_0"] + symbols["l__CODE_0"]
    initialized_end = symbols["s__INITIALIZED"] + symbols["l__INITIALIZED"]
    if any(size > 16384 for size in banks.values()) or fixed_end > 0x3e00 or initialized_end > 0xdc00:
        raise InputError("Linked bank/fixed-code/WRAM budget rejected")
    inspected = validate_rom(rom.read_bytes(), {"rom": {"banks": 128, "title": "BALATROM"},
                                               "budgets": {"fixed_reserve": 512, "stack_reserve": 1024}})
    result = {"rom": inspected, "compiled": compiled, "commands": commands, "banks": banks,
              "fixed_end": fixed_end, "initialized_WRAM_end": initialized_end,
              "cached_game_objects": sum(e["cached"] for e in compiled),
              "fresh_compiler_units": sum(not e["cached"] for e in compiled),
              "workers": workers, "timings_seconds": timings,
              "compile_link_wall_seconds": time.monotonic() - build_started}
    write_json(output / "LINK_RESULT.json", result)
    return result
