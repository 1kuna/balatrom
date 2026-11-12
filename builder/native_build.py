"""Compile admitted generated native units, not a complete game or ROM.

Reproduce source bytes from the same genuine input before using a private
workspace. A hand-edited receipt is never sufficient compilation admission.
No original Lua execution, engine fallback, installation or linker step.
"""
from pathlib import Path
import json
import os
import re
import shutil
import subprocess
import tempfile
from .archive import Archive, InputError, digest
from .conversion import convert_recipe
from .pipeline import verify_workspace
from .profiles import match, profile_hash, recipe_dependencies


def compile_native_units(game, workspace, output, compiler, *, profiles=None):
    output = Path(output).expanduser().absolute()
    if output.exists() or output.is_symlink():
        raise InputError("Native-check output must be a new private directory")
    compiler = Path(compiler).expanduser()
    if not compiler.is_file() or not os.access(compiler, os.X_OK):
        raise InputError("Select your installed GBDK lcc executable")
    compiler = compiler.resolve()
    receipt = verify_workspace(workspace, profiles=profiles)
    # Never rely on user-editable receipt hashes alone: derive bytes again.
    source_files = {}
    native_units = []
    with Archive(game) as archive:
        fingerprint = archive.fingerprint()
        profile = match(fingerprint, profiles)
        if fingerprint != receipt["input"] or profile_hash(profile) != receipt["profile_sha256"]:
            raise InputError("Native-check game input differs from imported workspace")
        for recipe in profile["recipes"]:
            if recipe["kind"] not in ("native-records", "native-pairs", "native-scenes"):
                raise InputError("Only banked native ABI recipes are admitted for unit compilation")
            inputs = {name: archive.read(name) for name in recipe_dependencies(recipe)}
            raw = inputs[recipe["member"]]
            files, details = convert_recipe(raw, recipe, inputs)
            for name, data in files.items():
                if (Path(workspace) / "generated" / name).read_bytes() != data:
                    raise InputError("Generated source does not reproduce from verified local input")
                source_files[name] = data
            native_units.extend({"source": name, "bank": details["source_banks"][name] if recipe["kind"] == "native-scenes" else recipe["bank"]}
                                for name in sorted(files) if name.endswith(".c"))
        archive.assert_unchanged()
    parent = output.parent.resolve(strict=True)
    output = parent / output.name
    workspace_root = Path(workspace).resolve(strict=True)
    selected = Path(game).expanduser().resolve(strict=True)
    if workspace_root == output or workspace_root in output.parents:
        raise InputError("Native-check output must be outside imported workspace")
    if selected.is_dir() and (selected == output or selected in output.parents):
        raise InputError("Native-check output cannot modify installation")
    temporary = Path(tempfile.mkdtemp(prefix=".native-check-", dir=parent))
    try:
        sources = temporary / "sources"
        objects = temporary / "objects"
        sources.mkdir(mode=0o700)
        objects.mkdir(mode=0o700)
        for name, data in source_files.items():
            (sources / name).write_bytes(data)
            (sources / name).chmod(0o600)
        records = []
        bank_totals = {}
        for unit in native_units:
            target = objects / (Path(unit["source"]).stem + ".o")
            # Explicit argument list: no shell, profile-provided flags or linker.
            # Only ordinary tool discovery/locale/temp settings are inherited.
            try:
                environment = {k: os.environ[k] for k in ("PATH", "TMPDIR", "LANG", "LC_ALL") if k in os.environ}
                result = subprocess.run([str(compiler), "-c", "-o", str(target), str(sources / unit["source"])],
                                        cwd=temporary, env=environment, capture_output=True, text=True, timeout=60)
            except (OSError, subprocess.TimeoutExpired) as exc:
                raise InputError("Native unit compiler failed or timed out") from exc
            if result.returncode or not target.is_file():
                raise InputError("Native unit compilation failed: " + unit["source"] + ": " + result.stderr[-1000:])
            raw = target.read_bytes()
            try:
                text = raw.decode("ascii")
            except UnicodeDecodeError as exc:
                raise InputError("Expected GBDK/SDCC relocatable object format") from exc
            areas = re.findall(r"(?m)^A (_CODE_\d+) size ([0-9A-Fa-f]+) ", text)
            bank_sizes = {name: int(size, 16) for name, size in areas}
            expected_area = "_CODE_" + str(unit["bank"])
            if expected_area not in bank_sizes or any(name != expected_area and size for name, size in bank_sizes.items()):
                raise InputError("Compiled native unit bank differs from recipe")
            if bank_sizes[expected_area] > 16384:
                raise InputError("Compiled native unit exceeds complete bank size")
            bank_totals[unit["bank"]] = bank_totals.get(unit["bank"], 0) + bank_sizes[expected_area]
            if bank_totals[unit["bank"]] > 16384:
                raise InputError("Combined generated units exceed their declared bank; repartition required")
            target.chmod(0o600)
            records.append({"source": unit["source"], "source_sha256": digest(source_files[unit["source"]]),
                            "object": "objects/" + target.name, "object_sha256": digest(raw),
                            "bank": unit["bank"], "bank_area_bytes": bank_sizes[expected_area]})
        report = {"schema": 1, "kind": "native-abi-unit-check", "profile_id": profile["id"],
                  "profile_sha256": profile_hash(profile), "input_manifest_sha256": fingerprint["manifest_sha256"],
                  "compiler_sha256": digest(compiler.read_bytes()), "units": records, "bank_area_bytes": bank_totals,
                  "rom_built": False, "engine_linked": False, "retail_fidelity_verified": False}
        (temporary / "native-check.json").write_text(json.dumps(report, indent=2) + "\n")
        (temporary / "native-check.json").chmod(0o600)
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
