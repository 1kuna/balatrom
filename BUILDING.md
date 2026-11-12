# Building BalatROM

This tree contains the engine inputs distributed in the 1.0.0 app. The build
also requires a supported local Balatro installation and the pinned compiler;
game archives, compiler binaries and ROMs are not included.

## Prerequisites

- Python 3.12 or newer. Use the same Python environment for installing dependencies
  and running commands. The builder does not install Python packages for you.
- Pillow 12.3.0: `python -m pip install --only-binary=:all: Pillow==12.3.0`
- GBDK 4.5.0 with SDCC 4.5.1 revision 15267. Use an existing installation;
  the CLI takes the full path to its `bin/lcc` (`lcc.exe` on Windows).
- Your Steam copy of Balatro 1.0.1o. It is read locally; its Lua is parsed as data,
  never executed. Other versions are refused.

## Build the ROM

From this repository, with a new output directory and an existing output parent:

```sh
mkdir -p build
python -m builder discover
python -m builder owned-build --game-path /path/to/Balatro.app \
  --engine-packet ./engine --lcc /path/to/gbdk/bin/lcc --output ./build/rom
```

On Windows, select the installed `Balatro.exe` and `lcc.exe` instead. The official
Windows SDCC executable is replaced in a temporary tool copy by a separately
verified compiler built from revision 15267. Its required hash and source/build
references are in `notices/reproducible-sdcc/SOURCE-CORRESPONDENCE.json`; compiler
binaries are not stored in this repository.

The result goes to `build/rom/linked/balatrom.gbc`. Version 1.0.0 must have SHA-256
`9f30e154e07e24085e0d0d106ed876df0cc29e9a517aa97cf75abc1a05972d01`.
Generated art, intermediate sources and the ROM stay local. Do not commit them.

Run the Python and desktop tests with:

```sh
python -m unittest discover -s tests -v
python -m unittest discover -s asset-authoring/tests -v
cd desktop
node --test test/*.test.cjs
```

The older Qt GUI tests also need PySide6-Essentials 6.11.2 and shiboken6 6.11.2;
Qt is not part of the released Electron app. Windows process and installer tests
need Windows. The save integration test skips unless its separately supplied
local fixture library is selected; no game fixtures are included here.

## Package the Mac app

First verify the ROM build above. You also need Node.js 22+,
Electron 45.0.0, PyInstaller 6.22.3 and a Python environment with Pillow 12.3.0.
The visible app is `desktop/`; the Python worker runs the same builder as the CLI.

Commit the source to be packaged. From the repository root, using the same Python
environment as the CLI build, run:

```sh
python -m pip install --only-binary=:all: PyInstaller==6.22.3
npm --prefix desktop ci
mkdir -p build
package_root="$(pwd)/build/macos"
mkdir "$package_root"
python tools/prepare_desktop_packet.py \
  --base-contract compatibility/initial-contract.json \
  --output "$package_root/packet"
python tools/package_desktop_worker.py --packet "$package_root/packet" \
  --output "$package_root/worker"
node desktop/package-app.cjs "$package_root/worker/dist/balatrom-worker" \
  "$package_root/BalatROM.app" \
  "$(pwd)/desktop/node_modules/electron/dist/Electron.app"
```

Use a new `package_root` for each run. The packager requires absolute paths;
an existing Electron 45.0.0 runtime may replace the final argument, with its
`LICENSE` and `LICENSES.chromium.html` beside `Electron.app`. The source ZIP uses
only Git-tracked files. Its contract records the app version, supported game
hashes, expected ROM hash and save compatibility and is bound to that exact ZIP.

These steps create `build/macos/BalatROM.app`, a local development app. Developer ID signing and notarization
are separate release steps; no signing credentials or profiles belong in Git.

## Package the Windows installer

Use a native Windows machine, the same shared source tree, .NET Framework 4.8
reference assemblies and the C# compiler. Installer source is in
`installer/windows/`. The executable embeds three resources:

- `Config`: start with `installer/windows/components.json`, which records the
  exact version, application folder, component URLs, sizes and hashes. Add
  `PayloadSha256` and `PackageManifestSha256` for the files you build.
- `Payload`: the app's own ZIP: Electron application files, Python worker and
  initial source/contract, notices, and separately supplied SDCC compiler and
  corresponding-source archive. Do not include a game archive or ROM.
- `Manifest`: the size and SHA-256 of every final application file, after the
  official runtime archives and payload have been combined.

Create those resources from the finished Windows app, then compile
`BootstrapCore.cs`, `BootstrapForm.cs`, `InstallerView.cs` and `SetupRunState.cs`
as an x64 Windows executable. Embed `app.manifest` and the three resources above;
reference System, System.Core, System.Xml, System.Drawing, System.Windows.Forms,
System.Net.Http, System.Runtime.Serialization, System.IO.Compression and
System.IO.Compression.FileSystem. The .NET compiler flags are `/target:winexe
/platform:x64 /optimize+ /deterministic+ /debug- /langversion:7.3`.

The existing desktop packager also supports `--platform win32 --plan`. Its native
assembly requires a hash-bound Windows worker/flow test result. This checkout
does not yet contain a complete Windows installer packaging recipe. In particular,
the shipped Windows launch path needs embedded Python, `bootstrap.py`, the
`backend/` tree and bundled tools; the Mac PyInstaller worker alone does not
supply that layout. Reconstructing that payload and generating its final manifest,
resource hashes and compiler inputs requires additional packaging tooling and
native Windows verification. No Windows package has been built from this tree.

## What the app and installer do

The app looks in Steam library manifests, or reads a game path you choose. It
reads the game archive and selected texture/catalog members, checks their hashes,
converts assets, and invokes local compiler tools. It does not launch Balatro,
execute its Lua, upload game files, or read Balatro's player saves.

On a Mac, the first Build downloads `gbdk-macos-arm64.tar.gz` from
`https://github.com/gbdk-2020/gbdk-2020/releases/download/4.5.0/`. The archive and
all extracted files must match `notices/GBDK-macos-arm64.json`. Downloads may
redirect only to GitHub's release-asset hosts. Nothing is added to the system PATH.

Windows setup downloads these official files after **Set up & open**:

| File | Source |
| --- | --- |
| `electron-v45.0.0-win32-x64.zip` | `github.com/electron/electron/releases/download/v45.0.0/` |
| `python-3.13.17-embed-amd64.zip` | `www.python.org/ftp/python/3.13.17/` |
| `pillow-12.3.0-cp313-cp313-win_amd64.whl` | `files.pythonhosted.org` |
| `gbdk-win64.zip` | `github.com/gbdk-2020/gbdk-2020/releases/download/4.5.0/` |

Full URLs, download sizes, SHA-256 hashes and extraction destinations are in
[`installer/windows/components.json`](installer/windows/components.json).

Setup verifies pinned lengths and SHA-256 hashes before extraction, checks every
application file, and opens `BalatROM.exe`. The existing 1.0.0 installer code uses
`%LOCALAPPDATA%\BalatROM\WindowsTest\apps\1.0.0` for the app and a sibling
`downloads` folder for its cache. That historical folder name is part of the
current installer behavior. A damaged existing folder is preserved before repair.
Setup requests ordinary user privileges; it does not install a system service,
change security settings or add PATH entries.

The app keeps versions, build tools, compiler cache and logs under its existing
user-data folder: `~/Library/Application Support/BalatROM Builder Preview 0.3.5-rc.8/`
on Mac and `%APPDATA%\BalatROM Builder Preview 0.3.5-rc.8\` on Windows. The old
folder name preserves existing installations. Export writes the ROM only to the
path you select and asks before overwriting an existing file.

See [THIRD-PARTY-NOTICES.md](THIRD-PARTY-NOTICES.md) for dependency licenses and
compiler-source delivery requirements.
