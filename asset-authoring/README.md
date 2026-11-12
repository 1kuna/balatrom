# Native assets

The BalatROM title and compact/regular fonts are original project assets. Their
geometric wordmark and explicit 3×5 / 5×7 cell recipes are in `tools/generate_title.py`
and `tools/generate_fonts.py`. They read no retail image, external font or game
archive. `tools/regenerate_ui_labels.py` draws those glyphs into the existing
native panel geometry. `desktop/make-icon.py` separately defines the app's
original geometric card monogram.

Generate title or font previews with Python and Pillow:

```sh
python asset-authoring/tools/generate_title.py --output /new/title-output
python asset-authoring/tools/generate_fonts.py --output /new/font-output
```

`tools/prepare_overlay.py --source-root /path/to/checkout --output /new/assets`
regenerates all eight native asset files and the static caption table into a new
directory. It checks its inputs and refuses to overwrite existing files or edit
the source tree. A changed asset needs updated source hashes and a fresh ROM
build; the fixed 1.0.0 ROM hash must not be silently changed.

Run `python -m unittest discover -s asset-authoring/tests -v` from the repository
root. The tests cover tile/map sizes, all seed values, font clipping, highlights,
UI glyph boundaries, input preservation and generated-file identity. They are
host tests, not a claim about physical Game Boy timing or game coverage.
