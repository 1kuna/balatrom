"""Atomic private output; each payload is derived from fingerprinted input."""
from pathlib import Path
import json
import shutil
import tempfile
from . import __version__
from .archive import Archive, InputError, digest
from .conversion import convert_recipe
from .profiles import match, profile_hash, recipe_outputs, recipe_dependencies, validate_outputs


def import_local(game, output, *, profiles=None):
    output = Path(output).expanduser().absolute()
    if output.exists() or output.is_symlink():
        raise InputError("Output must be a new private directory; existing files are never overwritten")
    parent = output.parent.resolve(strict=True)
    output = parent / output.name
    selected = Path(game).expanduser().resolve(strict=True)
    if selected.is_dir() and (output == selected or selected in output.parents):
        raise InputError("Output cannot be inside the game installation")
    with Archive(game) as archive:
        fingerprint = archive.fingerprint()
        profile = match(fingerprint, profiles)
        temporary = Path(tempfile.mkdtemp(prefix=".local-content-", dir=parent))
        try:
            generated = temporary / "generated"
            generated.mkdir(mode=0o700)
            rows = []
            names = set()
            for recipe in profile["recipes"]:
                inputs = {name: archive.read(name) for name in recipe_dependencies(recipe)}
                raw = inputs[recipe["member"]]
                if any(digest(data) != profile["members"][name]["sha256"] for name, data in inputs.items()):
                    raise InputError("Input changed after fingerprint validation")
                files, details = convert_recipe(raw, recipe, inputs)
                validate_outputs(recipe, files)
                for name, data in sorted(files.items()):
                    if name in names:
                        raise InputError("Generated output filenames collide")
                    names.add(name)
                    target = generated / name
                    target.write_bytes(data)
                    target.chmod(0o600)
                    rows.append({"path": "generated/" + name, "sha256": digest(data),
                                 "bytes": len(data), "input_member": recipe["member"],
                                 "input_sha256": digest(raw), "input_dependencies": {name: digest(data) for name, data in sorted(inputs.items())},
                                 "conversion": recipe, "details": details})
            receipt = {"schema": 1, "mode": "local-install", "builder_version": __version__,
                       "profile_id": profile["id"], "profile_sha256": profile_hash(profile),
                       "retail_version": profile["retail_version"], "input": fingerprint,
                       "generated": rows, "engine_admitted": False, "rom_built": False}
            (temporary / "receipt.json").write_text(json.dumps(receipt, indent=2) + "\n")
            (temporary / "receipt.json").chmod(0o600)
            (temporary / ".gitignore").write_text("*\n")
            archive.assert_unchanged()
            if output.exists() or output.is_symlink():
                raise InputError("Output appeared during conversion; refusing replacement")
            # rename is atomic, but use a reserved destination to prevent overwrite.
            output.mkdir(mode=0o700)
            try:
                temporary.rename(output)
            except Exception:
                output.rmdir()
                raise
            return receipt
        finally:
            if temporary.exists():
                shutil.rmtree(temporary)


def verify_workspace(output, *, profiles=None):
    root = Path(output)
    if root.is_symlink() or not root.is_dir():
        raise InputError("Select a regular private workspace")
    root = root.resolve()
    receipt_path = root / "receipt.json"
    if receipt_path.is_symlink():
        raise InputError("Receipt symlinks are unsupported")
    try:
        receipt = json.loads(receipt_path.read_text())
    except (OSError, ValueError) as exc:
        raise InputError("Private workspace has no valid receipt") from exc
    if (receipt.get("mode") != "local-install" or receipt.get("schema") != 1
            or receipt.get("builder_version") != __version__):
        raise InputError("Workspace mode/schema mismatch")
    from .profiles import registry
    profiles = [p for p in (registry() if profiles is None else profiles) if p["id"] == receipt.get("profile_id")]
    if len(profiles) != 1 or profile_hash(profiles[0]) != receipt.get("profile_sha256"):
        raise InputError("Workspace profile is not registered or has changed")
    profile = match(receipt.get("input", {}), profiles)
    rows = receipt.get("generated")
    expected = {}
    if not isinstance(rows, list):
        raise InputError("Invalid generated payload receipt")
    for recipe in profile["recipes"]:
        outputs = recipe_outputs(recipe)
        if outputs is None:
            outputs = [r.get("path", "").removeprefix("generated/") for r in rows
                       if isinstance(r, dict) and r.get("conversion") == recipe]
            validate_outputs(recipe, outputs)
        for name in outputs:
            if "generated/" + name in expected:
                raise InputError("Generated output contracts overlap")
            expected["generated/" + name] = recipe
    if not isinstance(rows, list) or len(rows) != len(expected):
        raise InputError("Incomplete generated payload receipt")
    seen = set()
    for row in rows:
        if not isinstance(row, dict):
            raise InputError("Invalid generated payload record")
        rel = row.get("path", "")
        if not isinstance(rel, str) or rel not in expected or rel in seen:
            raise InputError("Invalid generated receipt path")
        seen.add(rel)
        recipe = expected[rel]
        if (row.get("conversion") != recipe or row.get("input_member") != recipe["member"]
                or row.get("input_sha256") != profile["members"][recipe["member"]]["sha256"]
                or row.get("input_dependencies") != {name: profile["members"][name]["sha256"] for name in recipe_dependencies(recipe)}):
            raise InputError("Generated source provenance mismatch")
        path = root / rel
        if path.is_symlink() or path.parent.is_symlink() or not path.is_file():
            raise InputError("Generated input is missing or linked")
        data = path.read_bytes()
        if digest(data) != row.get("sha256") or len(data) != row.get("bytes"):
            raise InputError("Generated payload changed since conversion")
    actual = {p.relative_to(root).as_posix() for p in root.rglob("*") if p.is_file() or p.is_symlink()}
    if actual != seen | {"receipt.json", ".gitignore"}:
        raise InputError("Private workspace contains unrecorded files")
    return receipt
