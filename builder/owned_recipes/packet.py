"""Explicit private source/art packet with owner-derived payloads removed."""
from __future__ import annotations
from pathlib import Path
from typing import Any
import hashlib,json,re

NUMBER=re.compile(r'0x[0-9a-fA-F]+|\d+')
OWNER_SYMBOLS={'table_1998_jokers_0','table_1998_jokers_1','native_record_palettes_0','native_record_palettes_1','felt_item_tiles','native_item_palettes','native_negative_item_palettes','native_scene_card_palettes','a1_card_tiles','a1_card_maps','a1_card_ink_corners','de_sources','de_words','de_fix_keys','de_fix_words','np_palette_0','np_palette_1','dh_words'}
FULL_OWNER={'src/sidebar_jokers_0.c','src/sidebar_jokers_1.c','src/sidebar_planet.c','src/sidebar_tarot.c','src/sidebar_spectral.c','generated/retail_text_joker_names.inc','generated/retail_text_planet_labels.inc','generated/retail_text_tarot_labels.inc','generated/retail_text_spectral_labels.inc'}
def digest(raw: bytes) -> str:return hashlib.sha256(raw).hexdigest()
def create(reference: Path, inventory: dict[str,Any], ledger: dict[str,Any], output: Path, dependency: dict[str,Any]) -> dict[str,Any]:
 if output.exists():raise ValueError('Source packet destination must be new')
 output.mkdir(mode=0o700);(output/'.gitignore').write_text('*\n');rows=[];inputs=[];roles={x['path']:x for x in ledger['rows']}
 for entry in inventory['view_manifest']:
  rel=entry['path'];raw=(reference/rel).read_bytes()
  if digest(raw)!=entry['sha256']:raise ValueError('Reference changed: '+rel)
  row={'path':rel,'expected_assembled_sha256':entry['sha256'],'expected_bytes':len(raw),'roles':roles[rel]['roles'],'quoted_include_reachable':roles[rel]['quoted_include_reachable'],'rights_status':'private local authorization only; redistribution unreviewed','slots':[]}
  if rel in FULL_OWNER:row['supply']='fresh-owned-installation';row['packet_path']=None
  else:
   s=raw.decode();zero=roles[rel]['allzero_separated_fixture'];names=set(OWNER_SYMBOLS)
   if zero:names|=set(re.findall(r'\b(\w+)(?:\[[^]]*\])+\s*=\s*\{',s))
   for name in sorted(names):
    pattern=re.compile(r'\b'+re.escape(name)+r'(?:\[[^]]*\])+\s*=\s*\{(.*?)\};',re.S);m=pattern.search(s)
    if not m:continue
    body=m[1];tokens=list(NUMBER.finditer(body));formats=['hex:'+str(len(t[0])-2) if t[0].startswith('0x') else 'decimal' for t in tokens];count=len(tokens);skeleton=NUMBER.sub('@',body);recipe='owner:'+name if name in OWNER_SYMBOLS else 'zero:'+name
    if zero and name not in OWNER_SYMBOLS and any(int(t[0],0) for t in tokens):raise ValueError('Nonzero fixture cannot use zero recipe')
    marker='/* OWNER_INSTALL_SLOT:'+name+' */';s=s[:m.start(1)]+marker+s[m.end(1):];row['slots'].append({'symbol':name,'recipe':recipe,'count':count,'formats':formats,'skeleton':skeleton,'marker':marker})
   payload=s.encode();dest=output/'source'/rel;dest.parent.mkdir(parents=True,exist_ok=True);dest.write_bytes(payload);row['packet_path']='source/'+rel;row['packet_sha256']=digest(payload);row['supply']='explicit-private-source-with-fresh-generated-slots' if row['slots'] else 'explicit-private-source-or-authored-asset';inputs.append(row['packet_path'])
  rows.append(row)
 manifest={'schema':1,'scope':'private explicit retained source/authored asset packet; all known owner-derived pixel/name/palette slots omitted, no game objects or ROM; not export-admitted','descriptor':inventory['descriptor'],'descriptor_sha256':inventory['descriptor_sha256'],'rows':rows,'toolchain':dependency['build_and_link_programs']+dependency['linked_startup_and_archives'],'expected_reference_ROM_sha256':ledger['ROM_sha256'],'archive_sha256':'48c7a0791796a969d2cd0891ebdc9922b2988eb5aaad8ad7a72775a02772e24e','archive_manifest_sha256':'ae73502735ee949d8c74243469dee48663e4f52d4c6f9bdd61f81419d4cd5a2c','production_profile':False,'project_and_retained_source_license_cleared':False,'diagnostic_content_preserved':True}
 (output/'manifest.json').write_text(json.dumps(manifest,indent=2)+'\n');return manifest

def load(packet: Path) -> dict[str,Any]:
 if packet.is_symlink() or not packet.is_dir():raise ValueError('Explicit regular private source packet required')
 m=json.loads((packet/'manifest.json').read_text());expected={x['packet_path'] for x in m['rows'] if x['packet_path']}|{'manifest.json','.gitignore'}|set(m.get('additional_packet_inputs',[]))
 actual={p.relative_to(packet).as_posix() for p in packet.rglob('*') if p.is_file() or p.is_symlink()}
 if actual!=expected or any(p.is_symlink() for p in packet.rglob('*')):raise ValueError('Source packet has unrecorded/linked inputs; cached objects and ROMs are forbidden')
 for row in m['rows']:
  rel=Path(row['path'])
  if rel.is_absolute() or '..' in rel.parts or rel.suffix not in {'.c','.h','.inc','.s'}:raise ValueError('Invalid source input path')
  if row['packet_path'] and row['packet_path']!='source/'+row['path']:raise ValueError('Invalid explicit packet input path')
  if row['packet_path'] and digest((packet/row['packet_path']).read_bytes())!=row['packet_sha256']:raise ValueError('Private source input changed: '+row['path'])
 return m

def assemble(packet: Path,m: dict[str,Any],files: dict[str,bytes],arrays: dict[str,list[int]]) -> dict[str,bytes]:
 result={}
 for row in m['rows']:
  rel=row['path']
  if not row['packet_path']:
   if rel not in files:raise ValueError('Missing required owned recipe: '+rel)
   raw=files[rel]
  else:
   s=(packet/row['packet_path']).read_bytes().decode()
   for slot in row['slots']:
    values=arrays.get(slot['symbol']) if slot['recipe'].startswith('owner:') else [0]*slot['count']
    if values is None or len(values)!=slot['count']:raise ValueError('Owned recipe shape mismatch: '+slot['symbol'])
    it=iter(zip(values,slot['formats']))
    def value(_: re.Match[str]) -> str:
     n,fmt=next(it);return ('0x'+format(n,'0'+fmt.split(':')[1]+'x')) if fmt.startswith('hex:') else str(n)
    body=re.sub('@',value,slot['skeleton'])
    if s.count(slot['marker'])!=1:raise ValueError('Source slot is not unique')
    s=s.replace(slot['marker'],body)
   raw=s.encode()
  if digest(raw)!=row['expected_assembled_sha256'] or len(raw)!=row['expected_bytes']:raise ValueError('Fresh reconstruction differs from the expected source hash: '+rel)
  result[rel]=raw
 return result
