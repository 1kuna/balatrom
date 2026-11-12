import argparse
import json
import sys
from .archive import Archive, InputError
from .pipeline import import_local, verify_workspace
from .profiles import registry, match


def main(argv=None):
    argv = sys.argv[1:] if argv is None else argv
    if argv and argv[0] == "--update-worker":
        from .update_worker import main as worker_main
        return worker_main(argv[1:])
    if argv == ["app"]:
        print("Use the Electron desktop builder. The legacy Qt GUI entry is retired.", file=sys.stderr)
        return 2
    parser = argparse.ArgumentParser(description="Build BalatROM from your own copy of Balatro")
    commands = parser.add_subparsers(dest="command", required=True)
    commands.add_parser("compatibility", help="List supported game versions")
    discovery = commands.add_parser("discover", help="Find Balatro through Steam library manifests (never launches it)")
    discovery.add_argument("--steam-root", action="append", help="Explicit Steam root; repeat for custom library discovery")
    probe = commands.add_parser("probe", help="Read and fingerprint an ordinary game archive")
    probe.add_argument("--game-path", required=True)
    probe.add_argument("--include-members", action="store_true", help="Include private member metadata in stdout")
    importer = commands.add_parser("import-local", help="Convert a supported install into a new workspace")
    importer.add_argument("--game-path", required=True)
    importer.add_argument("--output", required=True)
    build = commands.add_parser("build", help="Check a workspace; does not build a ROM")
    build.add_argument("--workspace", required=True)
    native = commands.add_parser("check-native", help="Compile the generated units only; does not build a ROM")
    native.add_argument("--game-path", required=True)
    native.add_argument("--workspace", required=True)
    native.add_argument("--output", required=True)
    native.add_argument("--lcc", required=True)
    owned = commands.add_parser("owned-build", help="Build the ROM from your game install and the engine directory")
    owned.add_argument("--game-path", help="Explicit app/archive; otherwise discover through Steam manifests")
    owned.add_argument("--steam-root", action="append", help="Explicit Steam root for automatic discovery")
    owned.add_argument("--engine-packet", required=True)
    owned.add_argument("--output", required=True)
    owned.add_argument("--lcc", required=True)
    owned_mode = owned.add_mutually_exclusive_group()
    owned_mode.add_argument("--assemble-only", action="store_true", help="Verify source reconstruction without invoking the compiler")
    owned_mode.add_argument("--preflight", action="store_true", help="Check prerequisites read-only; no output directory, reconstruction or compiler execution")
    args = parser.parse_args(argv)
    try:
        if args.command == "owned-build":
            from .owned_install import build_owned, ARCHIVE_SHA, MANIFEST_SHA
            from .discovery import discover, select_supported
            game_path = args.game_path or select_supported(discover(args.steam_root), ARCHIVE_SHA, MANIFEST_SHA)
            receipt = build_owned(game_path, args.engine_packet, args.output, args.lcc,
                                  args.assemble_only, preflight=args.preflight)
            if args.preflight:
                print(json.dumps(receipt, indent=2))
            else:
                print(json.dumps({"profile_id": receipt["profile_id"], "ROM_built": receipt["ROM_built"],
                                  "ROM_sha256": receipt.get("ROM_sha256"), "public_export_admitted": False}, indent=2))
        elif args.command == "discover":
            from .discovery import discover
            print(json.dumps(discover(args.steam_root), indent=2))
        elif args.command == "compatibility":
            print(json.dumps({"verified_profiles": [{"id": p["id"], "retail_version": p["retail_version"]}
                                                    for p in registry()], "rom_build_available": False}, indent=2))
        elif args.command == "probe":
            with Archive(args.game_path) as archive:
                result = archive.fingerprint()
            try:
                profile = match(result)
                result.update(supported=True, profile_id=profile["id"])
            except InputError:
                result.update(supported=False, reason="No supported game version matches")
            result["member_count"] = len(result["members"])
            if not args.include_members:
                result.pop("members")
            print(json.dumps(result, indent=2))
            return 0 if result["supported"] else 2
        elif args.command == "import-local":
            receipt = import_local(args.game_path, args.output)
            print(json.dumps({"profile_id": receipt["profile_id"], "generated_files": len(receipt["generated"]),
                              "rom_built": False}, indent=2))
        elif args.command == "check-native":
            from .native_build import compile_native_units
            report = compile_native_units(args.game_path, args.workspace, args.output, args.lcc)
            print(json.dumps({"units_compiled": len(report["units"]), "rom_built": False}, indent=2))
        else:
            verify_workspace(args.workspace)
            raise InputError("This command does not build a ROM; use owned-build")
    except (InputError, OSError, ValueError) as exc:
        print("error: " + str(exc), file=sys.stderr)
        return 2
    return 0


if __name__ == "__main__":
    sys.exit(main())
