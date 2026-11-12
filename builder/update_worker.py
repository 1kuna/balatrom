"""Isolated build worker for verified release source. Installs nothing and never touches hardware."""
from __future__ import annotations
import argparse
import importlib.machinery
from pathlib import Path
import sys


def main(argv=None):
    parser = argparse.ArgumentParser()
    for name in ("source", "game", "output", "lcc", "cancel-file"):
        parser.add_argument("--" + name, required=True)
    parser.add_argument("--workers", type=int, choices=(1, 2, 3, 4), default=1)
    parser.add_argument("--object-cache", type=Path)
    args = parser.parse_args(argv)
    source = Path(args.source).resolve(strict=True)
    # Frozen app contains this supervisor, but the release source supplies builder code.
    sys.path.insert(0, str(source))
    for name in list(sys.modules):
        if name == "builder" or name.startswith("builder."):
            del sys.modules[name]
    class ReleaseSourceFinder:
        def find_spec(self, fullname, path=None, target=None):
            if fullname == "builder" or fullname.startswith("builder."):
                spec = importlib.machinery.PathFinder.find_spec(fullname, [str(source)] if path is None else path)
                if spec is None:
                    raise ImportError("Verified release source lacks required module: " + fullname)
                return spec
            return None
    # Prioritize the verified release package over a frozen app's bundled builder.
    sys.meta_path.insert(0, ReleaseSourceFinder())
    from builder.owned_install import build_owned
    def check():
        if Path(args.cancel_file).exists():
            raise RuntimeError("Build canceled; current version unchanged")
    build_owned(args.game, source / "engine", args.output, args.lcc, cancel_check=check, compile_workers=args.workers, object_cache=args.object_cache)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
