# Third-party notices

BalatROM's own code is licensed under the [MIT License](LICENSE). That license
does not cover Balatro's name, artwork, fonts, text or other game content.
Converted game content belongs to its respective owners and is created locally
from a user's own installation.

Dependency license texts are kept verbatim under `notices/`. They keep their own
terms, even when BalatROM is distributed under MIT.

| Component | License and notice location |
| --- | --- |
| GBDK 4.5.0 tools and libraries | `notices/gbdk/`; individual tools have different terms. `LICENSE_SDCC` includes library exceptions; `LICENSE_GPLV2_LE` contains the GPL library exception. |
| SDCC 4.5.1, revision 15267 | `notices/reproducible-sdcc/SDCC-LICENSE.txt`, `SDCC-COPYING.txt` and `COPYING`; compiler GPL terms are distinct from generated-program library exceptions. |
| LLVM-MinGW 20261020, LLVM libc++, MinGW-w64, winpthreads | `notices/reproducible-sdcc/LLVM-LICENSE.txt` and the `COPYING.MinGW*`, `COPYING.winpthreads.txt`, `COPYING.winstorecompat.txt` files. Apply the embedded-object and GPL exceptions only where their terms permit. |
| Boost 1.89.0 | `notices/reproducible-sdcc/BOOST-LICENSE.txt`. |
| Electron 45.0.0, Chromium and Node.js dependencies | `notices/electron/LICENSE` and `LICENSES.chromium.html`. Keep the notices supplied by the actual platform runtime when packaging. |
| Python 3.13.17 and incorporated code | `notices/Python-3.13.17-LICENSE.txt`, `PORTABLE-CPYTHON-LICENSE.txt` and `notices/windows-current/CPython-Doc-license.rst`. |
| Pillow 12.3.0 and bundled codecs | `notices/pillow-12.3.0.dist-info/licenses/LICENSE` and `notices/windows-current/Pillow-vendor-LICENSE.txt`. |
| OpenSSL, bzip2, Expat, libffi, SQLite, xz and zlib | Their named license files under `notices/windows-current/`, plus `notices/OpenSSL-3.5.9-LICENSE.txt`. |
| PyInstaller, its hooks, altgraph, macholib, packaging, pip and setuptools | Their versioned directories under `notices/`; PyInstaller's bootloader exception is in its `COPYING.txt`. |
| PySide6, Shiboken6 and Qt (legacy tests only) | `notices/Qt-LGPL-3.0.txt`, `Qt-pyside-LGPL-3.0.txt` and `Qt-GPL-exception.txt`. The Electron app excludes Qt. |

Pillow incorporates FreeType: copyright belongs to the FreeType Project authors
(https://freetype.org); its full terms are included in the Pillow notices.
This software is based in part on the work of the Independent JPEG Group.

The Windows Python runtime also contains Microsoft runtime DLLs. They retain
Microsoft's terms; the Python or BalatROM license does not relicense them.

## Compiler source with binary releases

The compiler source is attached to each release as stated in the README. A
binary release containing the rebuilt SDCC compiler must include its matching
configured source, applied patches and build scripts, with the required source
and notices for applicable linked components. Source URLs alone do not replace
those obligations. Do not substitute a different compiler build under the same
hash or omit its source attachment.

`notices/reproducible-sdcc/SOURCE-CORRESPONDENCE.json` records the rebuilt compiler
hash, upstream revision, vendor patches and LLVM-MinGW/Boost inputs. The official
GBDK download hashes and complete file inventories used by the app are in
`notices/GBDK-macos-arm64.json` and `notices/GBDK-windows-amd64.json`. The SDK cache
stays unchanged; only a disposable Windows execution copy receives the rebuilt
compiler. Compiler executables and source archives are supplied separately and
are not committed to this repository.

Official sources:

- [GBDK 4.5.0](https://github.com/gbdk-2020/gbdk-2020/tree/4.5.0)
- [GBDK SDCC patches](https://github.com/gbdk-2020/gbdk-2020-sdcc/tree/sdcc-patched-gbdk-4.5.0/Patches)
- [SDCC](https://sdcc.sourceforge.net/)
- [LLVM-MinGW 20261020](https://github.com/mstorsjo/llvm-mingw/releases/tag/20261020)
- [Boost 1.89.0](https://archives.boost.io/release/1.89.0/source/boost_1_89_0.tar.bz2)
- [Electron 45.0.0](https://github.com/electron/electron/tree/v45.0.0)
- [Python 3.13.17](https://github.com/python/cpython/tree/v3.13.17)
- [Pillow 12.3.0](https://github.com/python-pillow/Pillow/tree/12.3.0)
