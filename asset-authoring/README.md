# Native assets

The BalatROM title and both fonts (3×5 and 5×7) are original. The wordmark and
every glyph are drawn cell by cell in `tools/generate_title.py` and
`tools/generate_fonts.py`; neither reads a game image, font file or archive.
`tools/regenerate_ui_labels.py` draws those glyphs into the existing UI panels,
and `desktop/make-icon.py` draws the app icon.

Generate title or font previews with Python and Pillow:

```sh
python asset-authoring/tools/generate_title.py --output /new/title-output
python asset-authoring/tools/generate_fonts.py --output /new/font-output
```

`tools/prepare_overlay.py --source-root /path/to/checkout --output /new/assets`
regenerates all eight native asset files and the static caption table into a new
directory. It checks its inputs and won't overwrite files or touch the source
tree. A changed asset needs its source hashes updated and a fresh ROM build, and
the ROM will no longer match the 1.0.0 hash.

Run `python -m unittest discover -s asset-authoring/tests -v` from the repository
root. The tests cover tile/map sizes, every seed value, font clipping, highlights,
UI glyph boundaries, input preservation and that generated files match the
committed ones. They run on the host, not on Game Boy hardware.
