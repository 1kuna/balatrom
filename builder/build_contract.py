"""Portable separated-source compile/link contract, initially synthetic only.

No game engine or content payload is bundled. Source roles/hash bindings and
object order are explicit. Faithful admission is deliberately unavailable.
"""
from pathlib import Path
import json
import os
import re
import signal
import shutil
import subprocess
import tempfile
from .archive import InputError, canonical_json, digest


ROLES = {"authored-engine", "derived-content", "synthetic-test-data"}


def relative(name):
    if (not isinstance(name, str) or not re.fullmatch(r"[A-Za-z0-9_./-]+", name)
            or name.startswith("/") or any(p in ("", ".", "..", ".git") for p in name.split("/"))):
        raise InputError("Unsafe contract source path")
    return name


def validate_contract(contract):
    from .profiles import exact_keys, integer
    exact_keys(contract, ("schema", "purpose", "sources", "objects", "include_dirs", "rom", "budgets"))
    if contract["schema"] != 1 or contract["purpose"] != "synthetic-abi":
        raise InputError("Only synthetic ABI linking is admitted; faithful linking remains unavailable")
    exact_keys(contract["rom"], ("title", "banks"))
    if not isinstance(contract["rom"]["title"], str) or not re.fullmatch(r"[A-Z0-9_]{1,15}", contract["rom"]["title"]):
        raise InputError("Invalid synthetic ROM title")
    banks = contract["rom"]["banks"]
    if type(banks) is not int or banks not in (2, 4, 8, 16, 32, 64, 128):
        raise InputError("Unsupported ROM bank budget")
    exact_keys(contract["budgets"], ("fixed_reserve", "stack_reserve"))
    integer(contract["budgets"]["fixed_reserve"], 512, 8192)
    integer(contract["budgets"]["stack_reserve"], 512, 4096)
    includes = contract["include_dirs"]
    if not isinstance(includes, list) or len(includes) > 8 or any(not isinstance(d, str) for d in includes) or len(set(includes)) != len(includes):
        raise InputError("Invalid explicit include directories")
    for name in includes:
        relative(name)
    sources = contract["sources"]
    if not isinstance(sources, list) or not 1 <= len(sources) <= 512:
        raise InputError("Invalid contract source count")
    names = set()
    for entry in sources:
        exact_keys(entry, ("path", "sha256", "role", "input_members"))
        name = relative(entry["path"])
        if name in names or Path(name).suffix not in (".c", ".h"):
            raise InputError("Duplicate or unsupported contract source")
        names.add(name)
        if not isinstance(entry["sha256"], str) or not re.fullmatch(r"[0-9a-f]{64}", entry["sha256"]):
            raise InputError("Invalid contract source hash")
        if not isinstance(entry["role"], str) or entry["role"] not in ROLES:
            raise InputError("Unreviewed/unknown source origin is not admitted")
        inputs = entry["input_members"]
        if not isinstance(inputs, dict) or any(not isinstance(k, str) or not re.fullmatch(r"[0-9a-f]{64}", str(v)) for k, v in inputs.items()):
            raise InputError("Invalid source lineage binding")
        if (entry["role"] == "derived-content") != bool(inputs):
            raise InputError("Derived content needs local inputs; engine/test sources must remain separate")
    objects = contract["objects"]
    if not isinstance(objects, list) or not 1 <= len(objects) <= 197:
        raise InputError("Invalid ordered object count")
    ids, object_sources = set(), set()
    for unit in objects:
        exact_keys(unit, ("id", "source", "bank"))
        if not isinstance(unit["id"], str) or not re.fullmatch(r"[A-Za-z_][A-Za-z_0-9]{0,63}", unit["id"]) or unit["id"] in ids:
            raise InputError("Invalid or duplicate object ID")
        ids.add(unit["id"])
        if not isinstance(unit["source"], str) or unit["source"] not in names or not unit["source"].endswith(".c") or unit["source"] in object_sources:
            raise InputError("Each declared C source must compile exactly once")
        object_sources.add(unit["source"])
        integer(unit["bank"], 0, banks - 1)
    if object_sources != {name for name in names if name.endswith(".c")}:
        raise InputError("Ordered object list does not cover all C sources")
    return contract


def snapshot_sources(root, contract):
    root = Path(root).resolve(strict=True)
    expected = {entry["path"]: entry for entry in contract["sources"]}
    snapshots = {}
    for name, entry in expected.items():
        path = root / name
        if any(p.is_symlink() for p in [path, *list(path.parents)[:len(Path(name).parts) - 1]]):
            raise InputError("Contract source links are unsupported")
        data = path.read_bytes()
        if len(data) > 2 * 1024 * 1024 or digest(data) != entry["sha256"]:
            raise InputError("Contract source differs from pinned snapshot")
        try:
            text = data.decode("utf-8")
        except UnicodeDecodeError as exc:
            raise InputError("Contract source must be UTF-8") from exc
        # No include paths outside the explicitly hash-bound source graph.
        for match in re.finditer(r"(?m)^\s*#\s*include\s+([^\n]+)", text):
            token = match[1].strip()
            quoted = re.fullmatch(r'"([^"\n]+)"', token)
            system = re.fullmatch(r"<([^>\n]+)>", token)
            if quoted:
                relative(quoted[1])
                candidates = [str(Path(name).parent / quoted[1]), *[str(Path(d) / quoted[1]) for d in contract["include_dirs"]]]
                if not any(relative(dependency) in expected for dependency in candidates):
                    raise InputError("Include dependency is outside source manifest")
            elif not system or system[1] not in {"gb/gb.h", "gb/cgb.h", "stdint.h", "string.h"}:
                raise InputError("Unreviewed system/macro include")
        snapshots[name] = data
    return snapshots


def object_areas(raw):
    try:
        text = raw.decode("ascii")
    except UnicodeDecodeError as exc:
        raise InputError("Expected SDCC relocatable object") from exc
    rows = re.findall(r"(?m)^A (\S+) size ([0-9A-Fa-f]+) ", text)
    if not rows:
        raise InputError("Object has no allocation areas")
    return text, {name: int(size, 16) for name, size in rows}


def assign_bank(raw, bank):
    text, areas = object_areas(raw)
    if "_CODE_255" in areas:
        if bank == 0:
            raise InputError("Autobanked content cannot be assigned to fixed bank")
        text = re.sub(r"(?m)^(A )_CODE_255( )", r"\g<1>_CODE_%d\2" % bank, text)
        text = re.sub(r"(?m)^(S (?:b_|___bank_)\S+ Def)000000FF$",
                      lambda m: m[1] + "%08X" % bank, text)
        raw = text.encode()
    _, areas = object_areas(raw)
    allocated = {int(name[6:]): size for name, size in areas.items() if re.fullmatch(r"_CODE_\d+", name) and size}
    if any(number != bank for number in allocated):
        raise InputError("Explicit compiled bank conflicts with object manifest")
    if bank and not areas.get("_CODE_" + str(bank)):
        raise InputError("Banked object has no declared-bank allocation")
    if bank and areas.get("_CODE", 0):
        raise InputError("Banked object unexpectedly places code in fixed bank")
    return raw, areas


def validate_rom(raw, contract):
    if len(raw) != contract["rom"]["banks"] * 16384:
        raise InputError("Linked ROM size differs from declared bank budget")
    if raw[0x143] != 0xc0 or raw[0x147] != 0x1b or raw[0x149] != 2:
        raise InputError("Synthetic CGB/MBC5/RAM header differs from contract")
    if raw[0x148] != contract["rom"]["banks"].bit_length() - 2 or raw[0x134:0x143].rstrip(b"\0") != contract["rom"]["title"].encode("ascii"):
        raise InputError("ROM title/size header differs from contract")
    checksum = 0
    for value in raw[0x134:0x14d]:
        checksum = (checksum - value - 1) & 255
    if checksum != raw[0x14d]:
        raise InputError("ROM header checksum mismatch")
    global_sum = (sum(raw) - raw[0x14e] - raw[0x14f]) & 65535
    if global_sum != int.from_bytes(raw[0x14e:0x150], "big"):
        raise InputError("ROM global checksum mismatch")
    return {"bytes": len(raw), "sha256": digest(raw), "header_checksum": checksum, "global_checksum": global_sum}


def map_areas(text):
    rows = re.findall(r"(?m)^\s*([A-Za-z_][A-Za-z_0-9]*)\s+([0-9A-Fa-f]{8})\s+([0-9A-Fa-f]{8})\s+", text)
    return {name: {"address": int(address, 16), "bytes": int(size, 16)} for name, address, size in rows}


def validate_map(text, contract):
    areas = map_areas(text)
    if not areas:
        raise InputError("Linker map is missing allocation areas")
    for name, area in areas.items():
        size, address = area["bytes"], area["address"]
        if re.fullmatch(r"_CODE_\d+", name):
            if not 1 <= int(name[6:]) < contract["rom"]["banks"] or size > 16384 or not 0x4000 <= (address & 65535) <= 0x8000 or (address & 65535) + size > 0x8000:
                raise InputError("Linked switchable bank exceeds its window")
        elif name in ("_HOME", "_CODE", "_GSINIT", "_GSFINAL", "_HEADER", "_INITIALIZER"):
            if address + size > 0x4000 - contract["budgets"]["fixed_reserve"]:
                raise InputError("Linked fixed-bank reserve is exhausted")
        if name in ("_DATA", "_INITIALIZED", "_BSS") and size:
            if address < 0xc000 or address + size > 0xe000 - contract["budgets"]["stack_reserve"]:
                raise InputError("Linked WRAM allocation exhausts stack reserve")
    return areas


def compile_contract(root, contract, output, compiler, progress=None):
    validate_contract(contract)
    snapshots = snapshot_sources(root, contract)
    output = Path(output).expanduser().absolute()
    if output.exists() or output.is_symlink():
        raise InputError("Build output must be a new private directory")
    compiler = Path(compiler).expanduser().resolve(strict=True)
    if not compiler.is_file() or not os.access(compiler, os.X_OK):
        raise InputError("Select installed GBDK lcc")
    parent = output.parent.resolve(strict=True)
    output = parent / output.name
    temporary = Path(tempfile.mkdtemp(prefix=".synthetic-link-", dir=parent))
    try:
        for name, data in snapshots.items():
            path = temporary / name
            path.parent.mkdir(parents=True, exist_ok=True)
            path.write_bytes(data)
        (temporary / "objects").mkdir()
        env = {k: os.environ[k] for k in ("PATH", "TMPDIR", "LANG", "LC_ALL") if k in os.environ}

        def run(arguments):
            process = None
            try:
                process = subprocess.Popen([str(compiler), *arguments], cwd=temporary, env=env,
                                           stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True,
                                           start_new_session=(os.name == "posix"))
                stdout, stderr = process.communicate(timeout=60)
            except subprocess.TimeoutExpired as exc:
                if os.name == "posix":
                    try:
                        os.killpg(process.pid, signal.SIGKILL)
                    except ProcessLookupError:
                        pass
                else:
                    process.kill()
                process.communicate()
                raise InputError("Synthetic compiler/linker failed or timed out") from exc
            except OSError as exc:
                raise InputError("Synthetic compiler/linker failed or timed out") from exc
            if process.returncode:
                raise InputError("Synthetic compiler/linker rejected contract: " + stderr[-2000:])
            return stdout, stderr

        records, bank_bytes = [], {}
        for index, unit in enumerate(contract["objects"]):
            name = unit["id"]
            obj = "objects/" + name + ".o"
            run(["-msm83:gb", *["-I" + d for d in contract["include_dirs"]], "-c", "-o", obj, unit["source"]])
            raw = (temporary / obj).read_bytes()
            linked, areas = assign_bank(raw, unit["bank"])
            rel = "objects/" + name + ".rel"
            (temporary / rel).write_bytes(linked)
            size = areas.get("_CODE_" + str(unit["bank"]), 0) if unit["bank"] else sum(
                areas.get(k, 0) for k in ("_HOME", "_CODE", "_CODE_0", "_GSINIT", "_GSFINAL"))
            bank_bytes[unit["bank"]] = bank_bytes.get(unit["bank"], 0) + size
            limit = 16384 - (contract["budgets"]["fixed_reserve"] if unit["bank"] == 0 else 0)
            if bank_bytes[unit["bank"]] > limit:
                raise InputError("Ordered object allocation exceeds bank budget")
            records.append({**unit, "raw_sha256": digest(raw), "linked_sha256": digest(linked), "bytes": size})
            if progress and (index % 25 == 0 or index + 1 == len(contract["objects"])):
                progress(index + 1, len(contract["objects"]))
        (temporary / "objects.lk").write_text("".join("objects/" + unit["id"] + ".rel\n" for unit in contract["objects"]))
        run(["-msm83:gb", "-Wm-yC", "-Wm-yt0x1B", "-Wm-ya1", "-Wm-yo" + str(contract["rom"]["banks"]),
             "-Wm-yn" + contract["rom"]["title"], "-Wl-m", "-Wl-j", "-Wl-fobjects.lk", "-o", "synthetic-abi-check.gbc"])
        rom = validate_rom((temporary / "synthetic-abi-check.gbc").read_bytes(), contract)
        allocation = validate_map((temporary / "synthetic-abi-check.map").read_text(), contract)
        report = {"schema": 1, "purpose": "synthetic-abi", "contract_sha256": digest(canonical_json(contract)),
                  "compiler_sha256": digest(compiler.read_bytes()), "objects": records, "bank_bytes": bank_bytes,
                  "linked_areas": allocation, "rom": rom, "faithful_game_validated": False,
                  "retail_profiles_verified": 0, "hardware_used": False}
        (temporary / "BUILD_CONTRACT.json").write_text(json.dumps(contract, indent=2) + "\n")
        (temporary / "BUILD_REPORT.json").write_text(json.dumps(report, indent=2) + "\n")
        (temporary / ".gitignore").write_text("*\n")
        output.mkdir(mode=0o700)
        try:
            temporary.rename(output)
        except Exception:
            output.rmdir()
            raise
        return report
    finally:
        if temporary.exists():
            shutil.rmtree(temporary)
