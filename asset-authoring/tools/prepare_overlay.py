#!/usr/bin/env python3
"""Regenerate the native title, fonts and UI labels into a new directory.

Inputs are hash checked. The source tree and existing outputs are never edited.
"""
from __future__ import annotations
import argparse
import hashlib
import json
from pathlib import Path
import shutil
import tempfile
import types
import generate_fonts
import generate_title
import regenerate_ui_labels

EXPECTED = {
    "src/title_A_assets.inc": "f4df207abdb4a4c999e69c9dd9764848951fbe57bd9e9a8f94a0ed2d144f5859",
    "src/a1_title_runtime.c": "7ef63f2645b3e33f217647cf1ca085ee1132f0d2f745df1830cd877aed8fc8b5",
    "src/frontend_font_rows.inc": "1afb25bfbeedc0c099be2e8c2eb7587ffcf46c5644c6f86f8b120cd88894bf57",
    "generated/frontend_font5_font5.inc": "40bd74cf4a44ee6637b6eb14615fe3d9f9ebdd31001e445655ac67f84f00f683",
    "generated/a1_font_3_a1_font3_masks.inc": "25ed0e0b2f60b27ee07f0c55b3f5be47efe508f3cf645eab0b8ea56b395a6ad5",
    "generated/a1_font_5_a1_font5_masks.inc": "cdcefce5450fb7c7d4201b70b26cab0fb810c5d775621b9eb6e7989be7215ec9",
    "generated/a1_ui_assets_a1_ui_tiles.inc": "02746dbdb76f0384d6e940e61512f337ddb22ae117b6a531ac06d95c35c45d67",
    "generated/a1_ui_assets_a1_status_tiles.inc": "6996f4bb194e7037ea2f7124a5c4fbeb6b91c3208f211a486433ebe3be2aede7"
}
DERIVATION_INPUTS = {
    "generated/a1_ui_assets_a1_base_ids.inc": "ac9c019aa4e7327e6c955fe54f979661f10984667277b440b7dae52394dae16b",
    "generated/a1_ui_assets_a1_ui_tiles.inc": "02746dbdb76f0384d6e940e61512f337ddb22ae117b6a531ac06d95c35c45d67",
    "generated/a1_ui_assets_a1_template_ids.inc": "a3456e36d094a9dc6072af0ca10bacc6831c2add0ec79c4e53ea14d3659b31db",
    "generated/a1_ui_assets_a1_template_attrs.inc": "7b026a906d021757a285f23b01b14c0a41e115e0fdcdd0532ab88eb5f04e40b4",
    "src/R74_plain_templates.inc": "9531adf260a84e68510857916125e33d58a4171153343119d8740d7816faa803"
}

RECIPE_SHA = 'ba52e271bc1cf36b8a90dd056ecfe94c105df43b1764a90409c76618c8f67d66'

def sha(data):
    return hashlib.sha256(data).hexdigest()

def prepare(source_root: Path, destination: Path):
    source_root=source_root.resolve(strict=True)
    if destination.is_symlink():
        raise ValueError("Output must not be a symbolic link")
    destination=destination.resolve(strict=False)
    if destination==source_root or source_root in destination.parents:
        raise ValueError("Output must be outside the input source tree")
    if destination.exists() or destination.is_symlink():
        raise ValueError("Output already exists; choose a new isolated output directory")
    source=source_root/"engine/source"
    for relative,expected in (EXPECTED|DERIVATION_INPUTS).items():
        path=source/relative
        if path.is_symlink() or not path.is_file() or sha(path.read_bytes())!=expected:
            raise ValueError("Unsupported source hash: "+relative)
    recipe=source_root/"builder/owned_recipes/plain_templates.py"
    recipe_bytes=recipe.read_bytes()
    if recipe.is_symlink() or sha(recipe_bytes)!=RECIPE_SHA:
        raise ValueError("Unsupported cold-template recipe hash")

    data=generate_title.build()
    title="/* Original BalatROM native title. Title geometry is defined in asset-authoring/tools/generate_title.py.\n * No prior title image, game archive, or external font is read by this recipe.\n */\n"
    for name,key in [("title_A_tiles","tiles"),("title_A_map","mapping"),
                     ("title_A_attrs","attrs"),("title_A_resume_patch","patch")]:
        title+=generate_title.array(name,data[key])
    title+=generate_title.array("title_A_palettes",sum(generate_title.PALETTES,[]),"palette_color_t")
    title+=generate_title.array("balatrom_title_digits",sum(generate_title.DIGITS,[]))
    outputs=generate_fonts.build()
    ui_outputs,ui_receipt,_,_=regenerate_ui_labels.build(source)
    outputs.update(ui_outputs)
    outputs["src/title_A_assets.inc"]=title
    outputs["src/a1_title_runtime.c"]=(source/"src/a1_title_runtime.c").read_text()
    # The caption renderer consumes the same fonts and native UI geometry.
    with tempfile.TemporaryDirectory(prefix="balatrom-font-derive-") as tmp:
        packet=Path(tmp)/"engine"
        shutil.copytree(source,packet/"source")
        for relative,content in outputs.items():
            (packet/"source"/relative).write_text(content)
        # Execute only the already verified recipe bytes. An import directly
        # from the input source tree could write a __pycache__ there.
        module=types.ModuleType("verified_plain_templates")
        exec(compile(recipe_bytes,"verified_plain_templates.py","exec"),module.__dict__)
        derived=module.derive(packet)
    destination.mkdir(parents=True)
    rows=[]
    for relative,content in sorted(outputs.items()):
        path=destination/"engine/source"/relative
        path.parent.mkdir(parents=True,exist_ok=True)
        path.write_text(content)
        rows.append({"path":relative,"before_sha256":EXPECTED[relative],
                     "after_sha256":sha(path.read_bytes()),"bytes":path.stat().st_size})
    for relative,content in derived.items():
        path=destination/"derived-verification-only"/relative
        path.parent.mkdir(parents=True,exist_ok=True)
        path.write_bytes(content)
    receipt={"schema":1,"source_changes":rows,
             "derived_outputs":[{"path":p,"sha256":sha(v),"bytes":len(v)} for p,v in derived.items()],
             "originals_unchanged":all(sha((source/p).read_bytes())==v for p,v in (EXPECTED|DERIVATION_INPUTS).items())}
    (destination/"OVERLAY.json").write_text(json.dumps(receipt,indent=2)+"\n")
    return receipt

def main():
    ap=argparse.ArgumentParser()
    ap.add_argument("--source-root",type=Path,required=True)
    ap.add_argument("--output",type=Path,required=True)
    args=ap.parse_args()
    print(json.dumps(prepare(args.source_root,args.output),indent=2))

if __name__=="__main__":
    main()
